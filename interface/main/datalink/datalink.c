/**
 * @brief Data Link layer: bytes from the RS485 transceiver to/from data packets
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
#include "../network/network_hdr.h"
#include "tx_buf/tx_buf.h"
#include "ipc/ipc.h"
#include "datalink.h"

static char const * const TAG = "datalink_tx";

static uint8_t preamble_a5[] = { 0x00, 0xFF, 0xA5 };
static uint8_t preamble_ic[] = { 0x10, 0x02 };

/* MUST match the order of datalink_prot_t */

static struct proto_info_t {
	uint8_t const * const preamble;
	uint const len;
    datalink_prot_t const prot;
	uint idx;
} _proto_infos[] = {
	{
        .preamble = preamble_ic,
        .len = sizeof(preamble_ic),
        .prot = DATALINK_PROT_IC,
    },
	{
        .preamble = preamble_a5,
        .len = sizeof(preamble_a5),
        .prot = DATALINK_PROT_A5_CTRL,  // distinction between A5_CTRL and A5_PUMP is based on src/dst in hdr
    },
};

typedef enum DATALINK_RESULT_t {
    DATALINK_RESULT_DONE,
    DATALINK_RESULT_INCOMPLETE,
    DATALINK_RESULT_ERROR,
} DATALINK_RESULT_t;

static void
_preamble_reset()
{
	for (uint ii = 0; ii < ARRAY_SIZE(_proto_infos); ii++) {
		_proto_infos[ii].idx = 0;
	}
}

static bool
_preamble_is_done(struct proto_info_t * const pi, uint8_t const b, bool * found)
{
	if (b == pi->preamble[pi->idx]) {
		*found = true;
		pi->idx++;
		if (pi->idx == pi->len) {
			_preamble_reset();
			return true;
		}
	} else {
		*found = false;
	}
	return false;
}

static DATALINK_RESULT_t
_find_preamble(rs485_handle_t const rs485, datalink_prot_t * const proto)
{
    uint len = 0;
    uint buf_size = 40;
    char dbg[buf_size];

	while (rs485->available()) {
		uint8_t byt = rs485->read();
		if (CONFIG_POOL_DBG_DATALINK) {
            len += snprintf(dbg + len, buf_size - len, " %02X", byt);
		}
		bool found = false;  // could be the next byte of a preamble
		for (uint pp = 0; !found && pp < ARRAY_SIZE(_proto_infos); pp++) {
			if (_preamble_is_done(&_proto_infos[pp], byt, &found)) {
				*proto = _proto_infos[pp].prot;
				if (CONFIG_POOL_DBG_DATALINK) {
                    ESP_LOGI(TAG, "%s (preamble)", dbg);
				}
				return DATALINK_RESULT_DONE;
			}
		}
		if (!found) {  // or, could be the beginning of a preamble
			_preamble_reset();
			for (uint pp = 0; pp < ARRAY_SIZE(_proto_infos); pp++) {
				(void)_preamble_is_done(&_proto_infos[pp], byt, &found);
			}
		}
	}
	return DATALINK_RESULT_INCOMPLETE;
}

static DATALINK_RESULT_t
_read_header(rs485_handle_t const rs485, datalink_prot_t * const proto, datalink_hdr_t * const hdr)
{
	switch (*proto) {
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP:
			if (rs485->available() >= (int)sizeof(datalink_a5_hdr_t)) {
				rs485->read_bytes((uint8_t *)hdr, sizeof(datalink_a5_hdr_t));
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %02X %02X %02X %02X %02X (header)", hdr->ver, hdr->dst, hdr->src, hdr->typ, hdr->len);
				}
				if (hdr->len > CONFIG_POOL_DATALINK_LEN) {
					return DATALINK_RESULT_ERROR;  // pkt length exceeds what we have planned for
				}
                datalink_addrgroup_t src = datalink_groupaddr(hdr->src);
                datalink_addrgroup_t dst = datalink_groupaddr(hdr->dst);
    			if (src == DATALINK_ADDRGROUP_PUMP || dst == DATALINK_ADDRGROUP_PUMP) {
                    *proto = DATALINK_PROT_A5_PUMP;
                }
				return DATALINK_RESULT_DONE;
			}
			break;
		case DATALINK_PROT_IC:
			if (rs485->available() >= (int)sizeof(datalink_ic_hdr_t)) {
				datalink_ic_hdr_t hdr_ic;
				rs485->read_bytes((uint8_t *)&hdr_ic, sizeof(hdr_ic));
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %02X %02X (header)\n", hdr->dst, hdr->typ);
				}
                uint8_t len = network_ic_len(hdr_ic.typ);
                if (!len || len > CONFIG_POOL_DATALINK_LEN) {
                    return DATALINK_RESULT_ERROR;
                }
                // // so we can treat it as a A5 message when reading data or while decoding
                hdr->ver = 0x00;
                hdr->dst = hdr_ic.dst;
                hdr->src = 0x00;
                hdr->typ = hdr_ic.typ;
                hdr->len = len;
				return DATALINK_RESULT_DONE;
			}
	}
	return DATALINK_RESULT_INCOMPLETE;
}

static DATALINK_RESULT_t
_read_data(rs485_handle_t const rs485, datalink_hdr_t const * const hdr, uint8_t * const data)
{
    uint len = 0;
    uint buf_size = 80;
    char buf[buf_size];

	if (rs485->available() >= (int)sizeof(hdr->len)) {
		rs485->read_bytes((uint8_t *)data, hdr->len);
		if (CONFIG_POOL_DBG_DATALINK) {
			for (uint ii = 0; ii < hdr->len; ii++) {
                len += snprintf(buf + len, buf_size - len, " %02X", data[ii]);
			}
            ESP_LOGI(TAG, "%s (data)", buf);
		}
		return DATALINK_RESULT_DONE;
	}
	return DATALINK_RESULT_INCOMPLETE;
}

static DATALINK_RESULT_t
_read_crc(rs485_handle_t const rs485, datalink_prot_t const proto, uint16_t * chk)
{
	switch (proto) {
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP:
			if (rs485->available() >= (int)sizeof(uint16_t)) {
				*chk = rs485->read() << 8 | rs485->read();
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %03X (checksum)\n", *chk);
				}
				return DATALINK_RESULT_DONE;
			}
			break;

		case DATALINK_PROT_IC:
			if (rs485->available() >= (int)sizeof(uint8_t)) {
				*chk = rs485->read();  // store it as uint16_t, so we can treat it as A5 pkt
				if (CONFIG_POOL_DBG_DATALINK) {
					ESP_LOGI(TAG, " %02X (checksum)\n", *chk);
				}
				return DATALINK_RESULT_DONE;
			}
			break;
	}
	return DATALINK_RESULT_INCOMPLETE;
}

static uint16_t
_calc_crc_a5(datalink_hdr_t const * const hdr, uint8_t const * const data)
{
	// checksum is calculated starting at the last byte of the preamble (0xA5) up to the last
	// data byte
	uint16_t calc = preamble_a5[sizeof(preamble_a5) - 1];
	for (uint ii = 0; ii < sizeof(datalink_a5_hdr_t); ii++) {
		calc += ((uint8_t *)hdr)[ii];
	}
	if (data) {
		for (uint ii = 0; ii < hdr->len; ii++) {
			calc += data[ii];
		}
	}
	return calc;
}

static uint8_t
_calc_crc_ic(datalink_ic_hdr_t const * const hdr, uint8_t const * const data, size_t const data_len)
{
	// checksum is calculated starting at the preamble up to the last data byte
	uint8_t calc = hdr->dst + hdr->typ;  // pretend we still have the ic header
	for (uint ii = 0; ii < sizeof(preamble_ic); ii++) {
		calc += preamble_ic[ii];
	}
	for (uint ii = 0; ii < data_len; ii++) {
		calc += data[ii];
	}
	return calc;
}

static bool
_crc_is_correct(datalink_pkt_t * pkt)
{
	uint16_t calc = 0;  // init to please compiler
	switch (pkt->prot) {
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP:
			calc = _calc_crc_a5(&pkt->hdr, pkt->data);
			break;
		case DATALINK_PROT_IC: {
            datalink_ic_hdr_t hdr = {.dst = pkt->hdr.dst, .typ = pkt->hdr.typ};
			calc = _calc_crc_ic(&hdr, pkt->data, pkt->hdr.len);
			break;
        }
	}

	return calc == pkt->chk;
}

datalink_addrgroup_t
datalink_groupaddr(uint16_t const addr)
{
	return (datalink_addrgroup_t)(addr >> 4);
}

uint8_t
datalink_devaddr(uint8_t group, uint8_t const id)
{
	return (group << 4) | id;
}

uint32_t _millis() {
	return (uint32_t) (clock() * 1000 / CLOCKS_PER_SEC);
}

/**
 * Receive packets from the Pentair RS-485 bus.
 * Returns true when a complete packet has been received
 *
 * It relies on data from the network layer, for the correct length of the data pkt.
 */

bool
datalink_rx_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
    typedef enum STATE_t {
        STATE_FIND_PREAMBLE,
        STATE_READ_HDR,
        STATE_READ_DATA,
        STATE_READ_CRC,
        STATE_DONE,
    } STATE_t;

	time_t start = _millis();
	static STATE_t state = STATE_FIND_PREAMBLE;

	if (state != STATE_FIND_PREAMBLE && _millis() - start > CONFIG_POOL_RS485_TIMEOUT) {
		ESP_LOGW(TAG, "timeout\n");
		state = STATE_FIND_PREAMBLE;
	}
	DATALINK_RESULT_t result = DATALINK_RESULT_ERROR;  // init to please compiler
	switch (state) {
		case STATE_FIND_PREAMBLE:
            result = _find_preamble(rs485, &pkt->prot);
            break;
		case STATE_READ_HDR:
            result = _read_header(rs485, &pkt->prot, &pkt->hdr);
            break;
		case STATE_READ_DATA:
            result = _read_data(rs485, &pkt->hdr, pkt->data);
            break;
		case STATE_READ_CRC:
            result = _read_crc(rs485, pkt->prot, &pkt->chk);
            break;
		case STATE_DONE:
            break;
	}
	switch (result) {
		case DATALINK_RESULT_DONE:
			switch (state) {
				case STATE_FIND_PREAMBLE:
					start = _millis();
					state = STATE_READ_HDR;
					break;
				case STATE_READ_HDR:
					state = STATE_READ_DATA;
					break;
				case STATE_READ_DATA:
					state = STATE_READ_CRC;
					break;
				case STATE_READ_CRC:
					state = STATE_DONE;
					break;
				case STATE_DONE:
					break;
			}
			break;
		case DATALINK_RESULT_INCOMPLETE:
			break;
		case DATALINK_RESULT_ERROR:
			state = STATE_FIND_PREAMBLE;
			break;
	}

	if (state == STATE_DONE) {

		state = STATE_FIND_PREAMBLE;
        datalink_addrgroup_t const dst = datalink_groupaddr(pkt->hdr.dst);

        if ((pkt->prot == DATALINK_PROT_A5_CTRL && dst == DATALINK_ADDRGROUP_X09) ||
            (pkt->prot == DATALINK_PROT_IC && dst != DATALINK_ADDRGROUP_ALL && dst != DATALINK_ADDRGROUP_CHLOR)) {

                return false;  // silently ignore
        }
        return _crc_is_correct(pkt);
	}
	return false;
}

void
datalink_tx_pkt(rs485_handle_t const rs485_handle, tx_buf_handle_t const txb, datalink_prot_t const prot, datalink_ctrl_typ_t const typ)
{
    uint8_t * const data = txb->priv.data;
    size_t const data_len = txb->len;

    switch (prot) {
        case DATALINK_PROT_IC: {
            datalink_ic_hdr_t * const hdr = (datalink_ic_hdr_t *) tx_buf_push(txb, DATALINK_IC_HEAD_SIZE);
            hdr->dst = datalink_devaddr(DATALINK_ADDRGROUP_ALL, 0);
            hdr->typ = typ;
            uint8_t * crc = tx_buf_put(txb, DATALINK_IC_TAIL_SIZE);
            *crc = _calc_crc_ic(hdr, data, data_len);
            break;
        }
        case DATALINK_PROT_A5_CTRL:
        case DATALINK_PROT_A5_PUMP: {

// insert 0xFF + preamble here
#if 0
            rs485->write(0xFF);
            for (uint_least8_t ii = 0; ii < sizeof(preamble_a5); ii++) {
                rs485->write(preamble_a5[ii]);
            }

#endif

            datalink_a5_hdr_t * const hdr = (datalink_a5_hdr_t *) tx_buf_push(txb, DATALINK_A5_HEAD_SIZE);
            hdr->ver = 0x01;
            hdr->dst = datalink_devaddr(DATALINK_ADDRGROUP_CTRL, 0);
            hdr->src = datalink_devaddr(DATALINK_ADDRGROUP_REMOTE, 0);
            hdr->typ = typ;
            hdr->len = data_len;
            uint16_t const crcVal = _calc_crc_a5(hdr, data);
            uint8_t * crc = tx_buf_put(txb, DATALINK_A5_TAIL_SIZE);
            crc[0] = crcVal >> 8;
            crc[1] = crcVal & 0xFF;
            break;
        }
    }
    size_t const dbg_size = 128;
    char dbg[dbg_size];
    (void) tx_buf_print(TAG, txb, dbg, dbg_size);
    ESP_LOGI(TAG, "%s: { %s}", datalink_prot_str(prot), dbg);

    rs485_handle->queue(rs485_handle, txb);
}
