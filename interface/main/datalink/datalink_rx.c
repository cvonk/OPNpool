/**
 * @brief Data Link layer: bytes from the RS485 transceiver to data packets
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_err.h>
#include <esp_log.h>
#include <time.h>

#include "../rs485/rs485.h"
#include "../network/network.h"
#include "skb/skb.h"
#include "datalink.h"
#include "datalink_pkt.h"

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
    STATE_CHECK_CRC,
    STATE_DONE,
} state_t;

typedef struct local_data_t {
    size_t             head_len;
    size_t             tail_len;
    datalink_head_t *  head;
    datalink_tail_t *  tail;
    bool               crc_ok;
} local_data_t;

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

static esp_err_t
_find_preamble(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt)
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
                switch (pkt->prot) {
                    case DATALINK_PROT_A5_CTRL:
                    case DATALINK_PROT_A5_PUMP:
                        // add to pkt just in case we want to retransmit it
                        local->head->a5.ff = 0xFF;
                        preamble = local->head->a5.preamble;
                        local->head_len = sizeof(datalink_head_a5_t) ;
                        local->tail_len = sizeof(datalink_tail_a5_t) ;
                        break;
                    case DATALINK_PROT_IC:
                        preamble = local->head->ic.preamble;
                        local->head_len = sizeof(datalink_head_ic_t) ;
                        local->tail_len = sizeof(datalink_tail_ic_t) ;
                        break;
                }
                for (uint jj = 0; jj < info->len; jj++) {
                    preamble[jj] = info->preamble[jj];
                }
    			_preamble_reset();
				return ESP_OK;
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
	return ESP_FAIL;
}

static esp_err_t
_read_head(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt)
{
	switch (pkt->prot) {
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP: {
            datalink_hdr_a5_t * const hdr = &local->head->a5.hdr;
			if (rs485->read_bytes((uint8_t *) hdr, sizeof(datalink_hdr_a5_t)) == sizeof(datalink_hdr_a5_t)) {
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %02X %02X %02X %02X %02X (header)", hdr->ver, hdr->dst, hdr->src, hdr->typ, hdr->len);
				}
				if (hdr->len > DATALINK_MAX_DATA_SIZE) {
					return ESP_FAIL;  // pkt length exceeds what we have planned for
				}
    			if ( (datalink_groupaddr(hdr->src) == DATALINK_ADDRGROUP_PUMP) || (datalink_groupaddr(hdr->dst) == DATALINK_ADDRGROUP_PUMP) ) {
                    pkt->prot = DATALINK_PROT_A5_PUMP;
                }
                pkt->prot_typ = hdr->typ;
                pkt->src = hdr->src;
                pkt->dst = hdr->dst;
                pkt->data_len = hdr->len;
                if (pkt->data_len > sizeof(network_msg_data_a5_t)) {
                    return ESP_FAIL;
                }
				return ESP_OK;
			}
			break;
        }
		case DATALINK_PROT_IC: {
            datalink_hdr_ic_t * const hdr = &local->head->ic.hdr;
			if (rs485->read_bytes((uint8_t *) hdr, sizeof(datalink_hdr_ic_t)) == sizeof(datalink_hdr_ic_t)) {
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %02X %02X (header)", hdr->dst, hdr->typ);
				}
                pkt->prot_typ = hdr->typ;
                pkt->src = 0;
                pkt->dst = hdr->dst;
                pkt->data_len = network_ic_len(hdr->typ);
				return ESP_OK;
			}
        }
	}
	return ESP_FAIL;
}

static esp_err_t
_read_data(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt)
{
    uint len = 0;
    uint buf_size = 100;
    char buf[buf_size]; *buf = '\0';

	if (rs485->read_bytes((uint8_t *) pkt->data, pkt->data_len) == pkt->data_len) {
		if (CONFIG_POOL_DBG_DATALINK) {
			for (uint ii = 0; ii < pkt->data_len; ii++) {
                len += snprintf(buf + len, buf_size - len, " %02X", pkt->data[ii]);
			}
            ESP_LOGI(TAG, "%s (data)", buf);
		}
		return ESP_OK;
	}
	return ESP_FAIL;
}

static esp_err_t
_read_tail(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt)
{
	switch (pkt->prot) {
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP: {
            uint8_t * const crc = local->tail->a5.crc;
        	if (rs485->read_bytes(crc, sizeof(datalink_tail_a5_t)) == sizeof(datalink_tail_a5_t)) {
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %03X (checksum)", (uint16_t)crc[0] << 8 | crc[1]);
				}
				return ESP_OK;
			}
			break;
        }
		case DATALINK_PROT_IC: {
            uint8_t * const crc = local->tail->ic.crc;
        	if (rs485->read_bytes(crc, sizeof(datalink_tail_ic_t)) == sizeof(datalink_tail_ic_t)) {
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %02X (checksum)", crc[0]);
				}
				return ESP_OK;
			}
			break;
        }
	}
	return ESP_FAIL;
}

static esp_err_t
_check_crc(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt)
{
    struct {uint16_t rx, calc;} crc;
	switch (pkt->prot) {
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP: {
            crc.rx = (uint16_t)local->tail->a5.crc[0] << 8 | local->tail->a5.crc[1];
            uint8_t * const crc_start = &local->head->a5.preamble[sizeof(datalink_preamble_a5_t) - 1];  // starting at the last byte of the preamble
            uint8_t * const crc_stop = pkt->data + pkt->data_len;
            crc.calc = datalink_calc_crc(crc_start, crc_stop);
            break;
        }
		case DATALINK_PROT_IC: {
            crc.rx = local->tail->a5.crc[0];
            uint8_t * const crc_start = local->head->ic.preamble;  // starting at the first byte of the preamble
            uint8_t * const crc_stop = pkt->data + pkt->data_len;
            crc.calc = datalink_calc_crc(crc_start, crc_stop) & 0xFF;
            break;
        }
	}
    local->crc_ok = crc.rx == crc.calc;
    if (local->crc_ok) {
        return ESP_OK;
    }
    if (CONFIG_POOL_DBG_DATALINK_ONERROR) {
        ESP_LOGW(TAG, "crc err (rx=0x%03x calc=0x%03x)", crc.rx, crc.calc);
    }
    return ESP_FAIL;
}

/**
 * Receive packets from the Pentair RS-485 bus.
 * Returns true when a complete packet has been received
 *
 * It relies on data from the network layer, for the correct length of the data pkt.
 */

typedef esp_err_t (* state_fnc_t)(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt);

typedef struct state_transition_t {
    state_t     state;
    state_fnc_t fnc;
    state_t     on_ok;
    state_t     on_err;
} state_transition_t;

static state_transition_t state_transitions[] = {
    { STATE_FIND_PREAMBLE, _find_preamble, STATE_READ_HEAD,     STATE_FIND_PREAMBLE },
    { STATE_READ_HEAD,     _read_head,     STATE_READ_DATA,     STATE_FIND_PREAMBLE },
    { STATE_READ_DATA,     _read_data,     STATE_READ_TAIL,     STATE_FIND_PREAMBLE },
    { STATE_READ_TAIL,     _read_tail,     STATE_CHECK_CRC,     STATE_FIND_PREAMBLE },
    { STATE_CHECK_CRC,     _check_crc,     STATE_DONE,          STATE_FIND_PREAMBLE },
};

bool
datalink_rx_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
    state_t state = STATE_FIND_PREAMBLE;
    pkt->skb = skb_alloc(DATALINK_MAX_HEAD_SIZE + DATALINK_MAX_DATA_SIZE + DATALINK_MAX_TAIL_SIZE);
    local_data_t local;
    local.head = (datalink_head_t *) skb_put(pkt->skb, DATALINK_MAX_HEAD_SIZE);

    while (1) {
        state_transition_t * transition = state_transitions;
        for (uint ii = 0; ii < ARRAY_SIZE(state_transitions); ii++, transition++) {
            if (state == transition->state) {

                bool const ok = transition->fnc(rs485, &local, pkt) == ESP_OK;

                state_t const new_state = ok ? transition->on_ok : transition->on_err;
                switch (new_state) {
                    case STATE_FIND_PREAMBLE:
                        skb_reset(pkt->skb);
                        local.head = (datalink_head_t *) skb_put(pkt->skb, DATALINK_MAX_HEAD_SIZE);
                        break;
                    case STATE_READ_HEAD:
                        skb_call(pkt->skb, DATALINK_MAX_HEAD_SIZE - local.head_len);  // release unused bytes
                        break;
                    case STATE_READ_DATA:
                        pkt->data = (datalink_data_t *) skb_put(pkt->skb, pkt->data_len);
                        break;
                    case STATE_READ_TAIL:
                        local.tail = (datalink_tail_t *) skb_put(pkt->skb, local.tail_len);
                        break;
                    case STATE_CHECK_CRC:
                        break;
                    case STATE_DONE:
                        return local.crc_ok;
                        break;
                }
                state = new_state;
            }
        }
    }
}
