/**
 * @brief Data Link layer: bytes from the RS485 transceiver to data packets
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "../rs485/rs485.h"
#include "../network/network.h"
#include "skb/skb.h"
#include "datalink.h"

static char const * const TAG = "datalink_rx";

typedef struct proto_info_t {
	uint8_t const * const preamble;
	uint const len;
    datalink_prot_t const prot;
	uint idx;
} proto_info_t;

static proto_info_t _proto_descr[] = {
	{
        .preamble = datalink_preamble_ic,
        .len = sizeof(datalink_preamble_ic),
        .prot = DATALINK_PROT_IC,
    },
	{
        .preamble = datalink_preamble_a5,
        .len = sizeof(datalink_preamble_a5),
        .prot = DATALINK_PROT_A5_CTRL,  // distinction between A5_CTRL and A5_PUMP is based on src/dst in hdr
    },
};

typedef enum state_t {
    STATE_FIND_PREAMBLE,
    STATE_READ_HEAD,
    STATE_READ_DATA,
    STATE_READ_TAIL,
    STATE_DONE,
} state_t;

typedef enum state_result_t {
    STATE_RESULT_DONE,
    STATE_RESULT_ERROR,
} state_result_t;

static void
_preamble_reset()
{
    proto_info_t * info = _proto_descr;
	for (uint ii = 0; ii < ARRAY_SIZE(_proto_descr); ii++, info++) {
		info->idx = 0;
	}
}

static bool
_preamble_complete(struct proto_info_t * const pi, uint8_t const b, bool * part_of_preamble)
{
	if (b == pi->preamble[pi->idx]) {
		*part_of_preamble = true;
		pi->idx++;
		if (pi->idx == pi->len) {
			return true;
		}
	} else {
		*part_of_preamble = false;
	}
	return false;
}

static state_result_t
_find_preamble(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
    uint len = 0;
    uint buf_size = 40;
    char dbg[buf_size];

    uint8_t byt;

    while (rs485->read_bytes(&byt, 1) == 1) {
		if (CONFIG_POOL_DBG_DATALINK) {
            len += snprintf(dbg + len, buf_size - len, " %02X", byt);
		}
		bool part_of_preamble = false;
        proto_info_t * info = _proto_descr;
		for (uint ii = 0; !part_of_preamble && ii < ARRAY_SIZE(_proto_descr); ii++, info++) {
			if (_preamble_complete(info, byt, &part_of_preamble)) {
				if (CONFIG_POOL_DBG_DATALINK) {
                    ESP_LOGI(TAG, "%s (preamble)", dbg);
				}
				pkt->prot = info->prot;
                uint8_t * preamble = NULL;
                switch(pkt->prot) {
                    case DATALINK_PROT_A5_CTRL:
                    case DATALINK_PROT_A5_PUMP:
                        // add to pkt just in case we want to retransmit it
                        pkt->priv.head->a5.ff = 0xFF;
                        preamble = pkt->priv.head->a5.preamble;
                        pkt->priv.head_len = sizeof(datalink_head_a5_t) ;
                        pkt->priv.tail_len = sizeof(datalink_tail_a5_t) ;
                        break;
                    case DATALINK_PROT_IC:
                        preamble = pkt->priv.head->ic.preamble;
                        pkt->priv.head_len = sizeof(datalink_head_ic_t) ;
                        pkt->priv.tail_len = sizeof(datalink_tail_ic_t) ;
                        break;
                }
                for (uint jj = 0; jj < info->len; jj++) {
                    preamble[jj] = info->preamble[jj];
                }
    			_preamble_reset();
				return STATE_RESULT_DONE;
			}
		}
		if (!part_of_preamble) {  // could be the beginning of the next
			_preamble_reset();
            proto_info_t * info = _proto_descr;
			for (uint ii = 0; ii < ARRAY_SIZE(_proto_descr); ii++, info++) {
				(void)_preamble_complete(info, byt, &part_of_preamble);
			}
		}
	}
	return STATE_RESULT_ERROR;
}

static state_result_t
_read_head(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
	switch (pkt->prot) {
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP: {
            datalink_hdr_a5_t * const hdr = &pkt->priv.head->a5.hdr;
			if (rs485->available() >= (int)sizeof(datalink_hdr_a5_t)) {
				rs485->read_bytes((uint8_t *) hdr, sizeof(datalink_hdr_a5_t));
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %02X %02X %02X %02X %02X (header)", hdr->ver, hdr->dst, hdr->src, hdr->typ, hdr->len);
				}
				if (hdr->len > DATALINK_MAX_DATA_SIZE) {
					return STATE_RESULT_ERROR;  // pkt length exceeds what we have planned for
				}
    			if ( (datalink_groupaddr(hdr->src) == DATALINK_ADDRGROUP_PUMP) || (datalink_groupaddr(hdr->dst) == DATALINK_ADDRGROUP_PUMP) ) {
                    pkt->prot = DATALINK_PROT_A5_PUMP;
                }
                pkt->prot_typ = hdr->typ;
                pkt->src = hdr->src;
                pkt->dst = hdr->dst;
                pkt->data_len = hdr->len;
				return STATE_RESULT_DONE;
			}
			break;
        }
		case DATALINK_PROT_IC: {
            datalink_hdr_ic_t * const hdr = &pkt->priv.head->ic.hdr;
			if (rs485->available() >= (int)sizeof(datalink_hdr_ic_t)) {
				rs485->read_bytes((uint8_t *) hdr, sizeof(datalink_hdr_ic_t));
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %02X %02X (header)", hdr->dst, hdr->typ);
				}
                pkt->prot_typ = hdr->typ;
                pkt->src = 0;
                pkt->dst = hdr->dst;
                pkt->data_len = network_ic_len(hdr->typ);
				return STATE_RESULT_DONE;
			}
        }
	}
	return STATE_RESULT_ERROR;
}

static state_result_t
_read_data(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
    uint len = 0;
    uint buf_size = 80;
    char buf[buf_size];

	if (rs485->available() >= pkt->data_len) {
		rs485->read_bytes((uint8_t *) pkt->data, pkt->data_len);
		if (CONFIG_POOL_DBG_DATALINK) {
			for (uint ii = 0; ii < pkt->data_len; ii++) {
                len += snprintf(buf + len, buf_size - len, " %02X", pkt->data[ii]);
			}
            ESP_LOGI(TAG, "%s (data)", buf);
		}
		return STATE_RESULT_DONE;
	}
	return STATE_RESULT_ERROR;
}

static state_result_t
_read_tail(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
	switch (pkt->prot) {
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP: {
            uint8_t * const crc = pkt->priv.tail->a5.crc;
			if (rs485->available() >= (int)sizeof(uint16_t)) {
                crc[0] = rs485->read();
                crc[1] = rs485->read();
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %03X (checksum)", (uint16_t)crc[0] << 8 | crc[1]);
				}
				return STATE_RESULT_DONE;
			}
			break;
        }
		case DATALINK_PROT_IC: {
            uint8_t * const crc = pkt->priv.tail->a5.crc;
			if (rs485->available() >= (int)sizeof(uint8_t)) {
				crc[0] = rs485->read();  // store it as uint16_t, so we can treat it as A5 pkt
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %02X (checksum)", crc[0]);
				}
				return STATE_RESULT_DONE;
			}
			break;
        }
	}
	return STATE_RESULT_ERROR;
}

static bool
_crc_correct(datalink_pkt_t const * const pkt, uint16_t * const rx_crc, uint16_t * const calc_crc)
{
	switch (pkt->prot) {
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP: {
            *rx_crc = (uint16_t)pkt->priv.tail->a5.crc[0] << 8 | pkt->priv.tail->a5.crc[1];
            uint8_t * const crc_start = &pkt->priv.head->a5.preamble[sizeof(datalink_preamble_a5_t) - 1];  // starting at the last byte of the preamble
            uint8_t * const crc_stop = pkt->data + pkt->data_len;
            *calc_crc = datalink_calc_crc(crc_start, crc_stop);
            break;
        }
		case DATALINK_PROT_IC: {
            *rx_crc = pkt->priv.tail->a5.crc[0];
            uint8_t * const crc_start = pkt->priv.head->ic.preamble;  // starting at the first byte of the preamble
            uint8_t * const crc_stop = pkt->data + pkt->data_len;
            *calc_crc = datalink_calc_crc(crc_start, crc_stop) & 0xFF;
            break;
        }
	}
    return *calc_crc == *rx_crc;
}

static state_result_t
_done(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
    uint16_t rx_crc, calc_crc;
    pkt->priv.crc_ok = _crc_correct(pkt, &rx_crc, &calc_crc);
    if (pkt->priv.crc_ok) {
        return STATE_RESULT_DONE;
    }
    if (CONFIG_POOL_DBG_DATALINK_ONERROR) {
        ESP_LOGW(TAG, "checksum error (rx=0x%03x calc=0x%03x)", rx_crc, calc_crc);
    }
    return STATE_RESULT_ERROR;
}

/**
 * Receive packets from the Pentair RS-485 bus.
 * Returns true when a complete packet has been received
 *
 * It relies on data from the network layer, for the correct length of the data pkt.
 */

#if 0
bool
datalink_rx_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
	static time_t start = 0;  // 0 on first run

	if (start == 0 || (_state != STATE_FIND_PREAMBLE && _millis() - start > CONFIG_POOL_RS485_TIMEOUT)) {
        if (start == 0) {
            pkt->skb = skb_alloc(DATALINK_MAX_HEAD_SIZE + DATALINK_MAX_DATA_SIZE + DATALINK_MAX_TAIL_SIZE);
        } else if (CONFIG_POOL_DBG_DATALINK_ONERROR) {
    		ESP_LOGW(TAG, "timeout");
        }
        start = _millis();
		_change_state(pkt, STATE_FIND_PREAMBLE);
	}

	state_result_t result = STATE_RESULT_ERROR;  // init to please compiler
	switch (_state) {
		case STATE_FIND_PREAMBLE:
            result = _find_preamble(rs485, pkt);
            break;
		case STATE_READ_HEAD:
            result = _read_head(rs485, pkt);
            break;
		case STATE_READ_DATA:
            result = _read_data(rs485, pkt);
            break;
		case STATE_READ_TAIL:
            result = _read_tail(rs485, pkt);
            break;
		case STATE_DONE:
            break;
	}
	switch (result) {
		case STATE_RESULT_DONE:
			switch (_state) {
				case STATE_FIND_PREAMBLE:
					start = _millis();
					_change_state(pkt, STATE_READ_HEAD);
					break;
				case STATE_READ_HEAD:
					_change_state(pkt, STATE_READ_DATA);
					break;
				case STATE_READ_DATA:
					_change_state(pkt, STATE_READ_TAIL);
					break;
				case STATE_READ_TAIL:
					_change_state(pkt, STATE_DONE);
					break;
				case STATE_DONE:
					break;
			}
			break;
		case STATE_RESULT_ERROR:
			break;
		case STATE_RESULT_ERROR:
			_change_state(pkt, STATE_FIND_PREAMBLE);
			break;
	}

	if (_state == STATE_DONE) {

		_change_state(pkt, STATE_FIND_PREAMBLE);

        datalink_addrgroup_t const dst_a5 = datalink_groupaddr(pkt->priv.head->a5.hdr.dst);
        datalink_addrgroup_t const dst_ic = datalink_groupaddr(pkt->priv.head->ic.hdr.dst);

        if ((pkt->prot == DATALINK_PROT_A5_CTRL && dst_a5 == STATE_ADDRGROUP_X09) ||
            (pkt->prot == DATALINK_PROT_IC && dst_ic != STATE_ADDRGROUP_ALL && dst_ic != STATE_ADDRGROUP_CHLOR)) {
               return false;  // silently ignore
        }

        uint16_t rx_crc, calc_crc;
        bool const crc_correct = _crc_correct(pkt, &rx_crc, &calc_crc);
        if (CONFIG_POOL_DBG_DATALINK_ONERROR && !crc_correct) {
            ESP_LOGW(TAG, "checksum error (rx=0x%03x calc=0x%03x)", rx_crc, calc_crc);
        }
        return crc_correct;
	}
	return false;
}
#endif

typedef state_result_t (* state_fnc_t)(rs485_handle_t const rs485, datalink_pkt_t * const pkt);


typedef struct state_transition_t {
    state_t state;
    state_fnc_t fnc;
    state_t new_ok_state;
    state_t new_err_state;
} state_transition_t;

static state_transition_t state_transitions[] = {
    { STATE_FIND_PREAMBLE, _find_preamble, STATE_READ_HEAD,     STATE_FIND_PREAMBLE },
    { STATE_READ_HEAD,     _read_head,     STATE_READ_DATA,     STATE_FIND_PREAMBLE },
    { STATE_READ_DATA,     _read_data,     STATE_READ_TAIL,     STATE_FIND_PREAMBLE },
    { STATE_READ_TAIL,     _read_tail,     STATE_DONE,          STATE_FIND_PREAMBLE },
    { STATE_DONE,          _done,          STATE_FIND_PREAMBLE, STATE_FIND_PREAMBLE },
};

bool
datalink_rx_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
    state_t state = STATE_FIND_PREAMBLE;
    pkt->skb = skb_alloc(DATALINK_MAX_HEAD_SIZE + DATALINK_MAX_DATA_SIZE + DATALINK_MAX_TAIL_SIZE);

    while (1) {
        state_transition_t * transition = state_transitions;
        for (uint ii = 0; ii < ARRAY_SIZE(state_transitions); ii++, transition++) {
            if (state == transition->state) {
                bool const ok = transition->fnc(rs485, pkt) == STATE_RESULT_DONE;
                state_t const new_state = ok ? transition->new_ok_state : transition->new_err_state;
                switch (new_state) {
                    case STATE_FIND_PREAMBLE:
                        skb_reset(pkt->skb);
                        pkt->priv.head = (datalink_head_t *) skb_put(pkt->skb, DATALINK_MAX_HEAD_SIZE);
                        break;
                    case STATE_READ_HEAD:
                        skb_call(pkt->skb, DATALINK_MAX_HEAD_SIZE - pkt->priv.head_len);  // release unused bytes
                        break;
                    case STATE_READ_DATA:
                        pkt->data = (datalink_data_t *) skb_put(pkt->skb, pkt->data_len);
                        break;
                    case STATE_READ_TAIL:
                        pkt->priv.tail = (datalink_tail_t *) skb_put(pkt->skb, pkt->priv.tail_len);
                        break;
                    case STATE_DONE:
                        return pkt->priv.crc_ok;
                        break;
                }
                state = new_state;
            }
        }
    }
}
