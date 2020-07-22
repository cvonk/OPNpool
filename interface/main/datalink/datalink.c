/**
* @brief packet_task, packetizes RS485 byte stream from Pentair bus
 *
 * The Pentair controller uses two different protocols to communicate with its peripherals:
 *   - 	A5 has messages such as 0x00 0xFF <ldb> <sub> <dst> <src> <cfi> <len> [<data>] <chH> <ckL>
 *   -  IC has messages such as 0x10 0x02 <data0> <data1> <data2> .. <dataN> <ch> 0x10 0x03
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "rs485.h"
#include "datalink.h"
#include "../presentation/presentation.h"
#include "../presentation/decode.h"
#include "../ipc_msgs.h"
//#include "../state/poolstate.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "datalink";

static uint8_t preamble_a5[] = { 0x00, 0xFF, 0xA5 };
static uint8_t preamble_ic[] = { 0x10, 0x02 };

static struct proto_info_t {
	uint8_t const * const preamble;
	uint const len;
	uint idx;
} _proto_infos[] = {
	{
        .preamble = preamble_a5,
        .len = sizeof(preamble_a5),
    },
	{
        .preamble = preamble_ic,
        .len = sizeof(preamble_ic),
    },
};

typedef enum RESULT_t {
    RESULT_DONE,
    RESULT_INCOMPLETE,
    RESULT_ERROR,
} RESULT_t;

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

static RESULT_t
_find_preamble(rs485_handle_t const rs485, PROTOCOL_t * const proto)
{
	while (rs485->available()) {
		uint8_t b = rs485->read();
		if (CONFIG_POOL_DBG_RAW) {
			printf(" %02X", b);
		}
		bool found = false;  // could be the next byte of a preamble
		for (uint pp = 0; !found && pp < ARRAY_SIZE(_proto_infos); pp++) {
			if (_preamble_is_done(&_proto_infos[pp], b, &found)) {
				*proto = (PROTOCOL_t)pp;
				if (CONFIG_POOL_DBG_RAW) {
					printf(" (preamble)\n");
				}
				return RESULT_DONE;
			}
		}
		if (!found) {  // or, could be the beginning of a preamble
			_preamble_reset();
			for (uint pp = 0; pp < ARRAY_SIZE(_proto_infos); pp++) {
				(void)_preamble_is_done(&_proto_infos[pp], b, &found);
			}
		}
	}
	return RESULT_INCOMPLETE;
}

static RESULT_t
_read_header(rs485_handle_t const rs485, PROTOCOL_t const proto, mHdr_a5_t * const hdr)
{
	switch (proto) {
		case PROTOCOL_A5:
			if (rs485->available() >= (int)sizeof(mHdr_a5_t)) {
				rs485->read_bytes((uint8_t *)hdr, sizeof(mHdr_a5_t));
				if (CONFIG_POOL_DBG_RAW) {
					printf(" %02X %02X %02X %02X %02X (header)\n", hdr->pro, hdr->dst, hdr->src, hdr->typ, hdr->len);
				}
				if (hdr->len > CONFIG_POOL_DATALINK_LEN) {
					return RESULT_ERROR;  // msg length exceeds what we have planned for
				}
				return RESULT_DONE;
			}
			break;
		case PROTOCOL_IC:
			if (rs485->available() >= (int)sizeof(mHdr_ic_t)) {
				mHdr_ic_t hdr_ic;
				rs485->read_bytes((uint8_t *)&hdr_ic, sizeof(hdr_ic));
				if (CONFIG_POOL_DBG_RAW) {
					printf(" %02X %02X (header)\n", hdr->dst, hdr->typ);
				}
                uint8_t len = network_ic_len(hdr_ic.typ);
                if (!len) {
                    return RESULT_ERROR;
                }
                hdr->pro = 0x00;
                hdr->dst = hdr_ic.dst;
                hdr->src = 0x00;
                hdr->typ = hdr_ic.typ;
                hdr->len = len;
#if 0
				*hdr = {  // so we can treat it as a A5 message when reading data or while decoding
					.pro = 0x00,
					.dst = hdr_ic.dst,
					.src = 0x00,
					.typ = hdr_ic.typ,
					.len = len
				};
#endif
				if (hdr->len > CONFIG_POOL_DATALINK_LEN) {
					return RESULT_ERROR;  // msg length exceeds what we have planned for
				}
				return RESULT_DONE;
			}
	}
	return RESULT_INCOMPLETE;
}

static RESULT_t
_read_data(rs485_handle_t const rs485, mHdr_a5_t const * const hdr, uint8_t * const data)
{
	if (rs485->available() >= (int)sizeof(hdr->len)) {
		rs485->read_bytes((uint8_t *)data, hdr->len);
		if (CONFIG_POOL_DBG_RAW) {
			for (uint ii = 0; ii < hdr->len; ii++) {
				printf(" %02X", data[ii]);
			}
			printf(" (data)\n");
		}
		return RESULT_DONE;
	}
	return RESULT_INCOMPLETE;
}

static RESULT_t
_read_crc(rs485_handle_t const rs485, PROTOCOL_t const proto, uint16_t * chk)
{
	switch (proto) {
		case PROTOCOL_A5:
			if (rs485->available() >= (int)sizeof(uint16_t)) {
				*chk = rs485->read() << 8 | rs485->read();
				if (CONFIG_POOL_DBG_RAW) {
					printf(" %03X (checksum)\n", *chk);
				}
				return RESULT_DONE;
			}
			break;

		case PROTOCOL_IC:
			if (rs485->available() >= (int)sizeof(uint8_t)) {
				*chk = rs485->read();  // store it as uint16_t, so we can treat it as A5 msg
				if (CONFIG_POOL_DBG_RAW) {
					printf(" %02X (checksum)\n", *chk);
				}
				return RESULT_DONE;
			}
			break;
	}
	return RESULT_INCOMPLETE;
}

#if 0
static void
_checksumToJson(JsonObject * root, char const * const key, uint16_t const rx, uint16_t const calc)
{
	JsonObject & obj = root->createNestedObject(key);
	obj["rx"] = Utils::strHex16(rx);
	obj["ca"] = Utils::strHex16(calc);
}
#endif

static uint16_t
_calc_crc_a5(mHdr_a5_t const * const hdr, uint8_t const * const data)
{
	// checksum is calculated starting at the last byte of the preamble (0xA5) up to the last
	// data byte
	uint16_t calc = preamble_a5[sizeof(preamble_a5) - 1];
	for (uint ii = 0; ii < sizeof(mHdr_a5_t); ii++) {
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
_calc_crc_ic(mHdr_a5_t const * const hdr, uint8_t const * const data)
{
	// checksum is calculated starting at the preamble up to the last data byte
	uint8_t calc = hdr->dst + hdr->typ;  // pretend we still have the ic header
	for (uint ii = 0; ii < sizeof(preamble_ic); ii++) {
		calc += preamble_ic[ii];
	}
	for (uint ii = 0; ii < hdr->len; ii++) {
		calc += data[ii];
	}
	return calc;
}

static bool
_crc_is_correct(datalink_msg_t * msg)
{
	uint16_t calc = 0;  // init to please compiler
	switch (msg->proto) {
		case PROTOCOL_A5:
			calc = _calc_crc_a5(&msg->hdr, msg->data);
			break;
		case PROTOCOL_IC:
			calc = _calc_crc_ic(&msg->hdr, msg->data);
			break;
	}

	return calc == msg->chk;
}

uint32_t _millis() {
	return (uint32_t) (clock() * 1000 / CLOCKS_PER_SEC);
}

/**
 * Receive messages from the Pentair RS-485 bus.
 * Returns true when a complete message has been received (a transmit opportunity)
 */

static bool
_receive_packet(rs485_handle_t const rs485, datalink_msg_t * msg)
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
		printf("TIMEOUT\n");
		state = STATE_FIND_PREAMBLE;
	}
	RESULT_t result = RESULT_ERROR;  // init to please compiler
	switch (state) {
		case STATE_FIND_PREAMBLE:
            result = _find_preamble(rs485, &msg->proto);
            break;
		case STATE_READ_HDR:
            result = _read_header(rs485, msg->proto, &msg->hdr);
            break;
		case STATE_READ_DATA:
            result = _read_data(rs485, &msg->hdr, msg->data);
            break;
		case STATE_READ_CRC:
            result = _read_crc(rs485, msg->proto, &msg->chk);
            break;
		case STATE_DONE:
            break;
	}
	switch (result) {
		case RESULT_DONE:
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
					break;  // dummy
			}
			break;
		case RESULT_INCOMPLETE:
			break;
		case RESULT_ERROR:
			state = STATE_FIND_PREAMBLE;
			break;
	}

	if (state == STATE_DONE) {
		state = STATE_FIND_PREAMBLE;
        // 2BD: if checksum err, print as raw hex
        return _crc_is_correct(msg);
	}
	return false;
}

static bool
_good_time_to_tx(datalink_msg_t * msg)
{
    return (msg->proto == PROTOCOL_A5) &&
        (network_group_addr(msg->hdr.src) == ADDRGROUP_CTRL) &&
        (network_group_addr(msg->hdr.dst) == ADDRGROUP_ALL);
}

void
datalink_task(void * ipc_void)
{
 	//ipc_t * const ipc = ipc_void;

    rs485_config_t rs485_config = {
        .rx_pin = CONFIG_POOL_RS485_RXPIN,
        .tx_pin = CONFIG_POOL_RS485_TXPIN,
        .rts_pin = CONFIG_POOL_RS485_RTSPIN,
        .uart_port = 2,
        .uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
            .use_ref_tick = false,
        }
    };
    rs485_handle_t rs485_handle = rs485_init(&rs485_config);

    datalink_msg_t datalink_msg; // poolstate_t state;

    while (1) {
        if (_receive_packet(rs485_handle, &datalink_msg)) {

            ESP_LOGI(TAG, "received datalink pkt");

            network_msg_t network_msg;
            network_decode(&datalink_msg, &network_msg);
            if (network_msg.typ == NETWORK_MSGTYP_NONE) {
                ESP_LOGW(TAG, "received network pkt w/ errs");
                // 2BD: print as raw hex
        	} else {

                ESP_LOGI(TAG, "received network pkt");

                //interpetate and update the poolstate accordingly
                //poolstate_set(&poolstate);
            }

            if (_good_time_to_tx(&datalink_msg)) {
                // read incoming mailbox for things to transmit
                //   and transmit them
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}