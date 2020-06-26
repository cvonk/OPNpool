/**
 * Receive messages from the Pentair RS-485 bus
 * 
 * The Pentair controller uses two different protocols to communicate with its peripherals:
 *   - 	A5 has messages such as 0x00 0xFF <ldb> <sub> <dst> <src> <cfi> <len> [<data>] <chH> <ckL>
 *   -  IC has messages such as 0x10 0x02 <data0> <data1> <data2> .. <dataN> <ch> 0x10 0x03
 */

#include <time.h>
#include <sdkconfig.h>
#include "ArduinoJson.h"
#include "rs485.h"
#include "pooltypes.h"
#include "utils.h"
#include "decode.h"
#include "poolstate.h"
#include "receiver.h"

static uint8_t preamble_a5[] = { 0x00, 0xFF, 0xA5 };
static uint8_t preamble_ic[] = { 0x10, 0x02 };

static struct protocolInfo_t {
	uint8_t const * const preamble;
	uint_least8_t const len;
	uint_least8_t idx;
} g_protocolInfos[kProtocolCount] = {
	{ preamble_a5, sizeof(preamble_a5), 0 },
	{ preamble_ic, sizeof(preamble_ic), 0 },
};

static void
_preambleIdxReset()
{
	for (uint_least8_t ii = 0; ii < kProtocolCount; ii++) {
		g_protocolInfos[ii].idx = 0;
	}
}

static bool
_preambleDone(struct protocolInfo_t * const pi, uint8_t const b, bool * found)
{
	if (b == pi->preamble[pi->idx]) {
		*found = true;
		pi->idx++;
		if (pi->idx == pi->len) {
			_preambleIdxReset();
			return true;
		}
	}
	else {
		*found = false;
	}
	return false;
}

static result_t
_findPreamble(Rs485 * rs485, protocol_t * const proto)
{
	while (rs485->available()) {
		uint8_t b = rs485->read();
		if (CONFIG_POOL_DBG_RAW) {
			printf(" %02X", b);
		}

		bool found = false;  // could be the next byte of a preamble
		for (uint_least8_t pp = 0; !found && pp < kProtocolCount; pp++) {
			if (_preambleDone(&g_protocolInfos[pp], b, &found)) {
				*proto = static_cast<protocol_t>(pp);
				if (CONFIG_POOL_DBG_RAW) {
					printf(" (preamble)\n");
				}
				return RESULT_done;
			}
		}
		if (!found) {  // or, could be the beginning of a preamble
			_preambleIdxReset();
			for (uint_least8_t pp = 0; pp < kProtocolCount; pp++) {
				(void)_preambleDone(&g_protocolInfos[pp], b, &found);
			}
		}
	}
	return RESULT_incomplete;
}

static result_t
_readHeader(Rs485 * rs485, protocol_t const proto, mHdr_a5_t *  hdr)
{
	switch (proto) {

		case PROTOCOL_a5:
			if (rs485->available() >= (int)sizeof(mHdr_a5_t)) {
				rs485->readBytes((uint8_t *)hdr, sizeof(mHdr_a5_t));
				if (CONFIG_POOL_DBG_RAW) {
					printf(" %02X %02X %02X %02X %02X (header)\n", hdr->pro, hdr->dst, hdr->src, hdr->typ, hdr->len);
				}
				if (hdr->len > CONFIG_POOL_MSGDATA_BUFSIZE) {
					return RESULT_error;  // msg length exceeds what we have planned for
				}
				return RESULT_done;
			}
			break;

		case PROTOCOL_ic:
			if (rs485->available() >= (int)sizeof(mHdr_ic_t)) {
				mHdr_ic_t hdr_ic;
				rs485->readBytes((uint8_t *)&hdr_ic, sizeof(hdr_ic));
				if (CONFIG_POOL_DBG_RAW) {
					printf(" %02X %02X (header)\n", hdr->dst, hdr->typ);
				}
				uint8_t len;
				switch (hdr_ic.typ) {
					case MT_CHLOR_pingReq:    len = sizeof(mChlorPingReq_ic_t); break;
					case MT_CHLOR_ping:       len = sizeof(mChlorPing_ic_t);    break;
					case MT_CHLOR_name:       len = sizeof(mChlorName_ic_t);    break;
					case MT_CHLOR_lvlSet:     len = sizeof(mChlorLvlSet_ic_t);  break;
					case MT_CHLOR_lvlSetResp: len = sizeof(mChlorPing_ic_t);    break;
					case MT_CHLOR_0x14:       len = sizeof(mChlor0X14_ic_t);    break;
					default: return RESULT_error;
				}
				*hdr = {  // so we can treat it as a A5 message when reading data or while decoding
					.pro = 0x00,
					.dst = hdr_ic.dst,
					.src = 0x00,
					.typ = hdr_ic.typ,
					.len = len
				};
				if (hdr->len > CONFIG_POOL_MSGDATA_BUFSIZE) {
					return RESULT_error;  // msg length exceeds what we have planned for
				}
				return RESULT_done;
			}
	}
	return RESULT_incomplete;
}

static result_t
_readData(Rs485 * rs485, mHdr_a5_t const * const hdr, uint8_t * const data)
{
	if (rs485->available() >= (int)sizeof(hdr->len)) {
		rs485->readBytes((uint8_t *)data, hdr->len);
		if (CONFIG_POOL_DBG_RAW) {
			for (uint_least8_t ii = 0; ii < hdr->len; ii++) {
				printf(" %02X", data[ii]);
			}
			printf(" (data)\n");
		}
		return RESULT_done;
	}
	return RESULT_incomplete;
}

static result_t
_readCRC(Rs485 * rs485, protocol_t const proto, uint16_t * chk)
{
	switch (proto) {
		case PROTOCOL_a5:
			if (rs485->available() >= (int)sizeof(uint16_t)) {
				*chk = rs485->read() << 8 | rs485->read();
				if (CONFIG_POOL_DBG_RAW) {
					printf(" %03X (checksum)\n", *chk);
				}
				return RESULT_done;
			}
			break;

		case PROTOCOL_ic:
			if (rs485->available() >= (int)sizeof(uint8_t)) {
				*chk = rs485->read();  // store it as uint16_t, so we can treat it as A5 msg
				if (CONFIG_POOL_DBG_RAW) {
					printf(" %02X (checksum)\n", *chk);
				}
				return RESULT_done;
			}
			break;
	}
	return RESULT_incomplete;
}

static void
_checksumToJson(JsonObject * root, char const * const key, uint16_t const rx, uint16_t const calc)
{
	JsonObject & obj = root->createNestedObject(key);
	obj["rx"] = Utils::strHex16(rx);
	obj["ca"] = Utils::strHex16(calc);
}

static bool 
_verifyCRC(pentairMsg_t * msg, JsonObject * json)
{
	uint16_t calc = 0;  // init to please compiler
	switch (msg->proto) {
		case PROTOCOL_a5:
			calc = Utils::calcCRC_a5(&msg->hdr, msg->data);
			break;
		case PROTOCOL_ic:
			calc = Utils::calcCRC_ic(&msg->hdr, msg->data);
			break;
	}

	bool const correct = (calc == msg->chk);
	if (!correct) {
		_checksumToJson(json, "checksum", msg->chk, calc);
	}
	return correct;
}

uint32_t _millis() {
	return (uint32_t) (clock() * 1000 / CLOCKS_PER_SEC);
}

/**
 * Receive messages from the Pentair RS-485 bus.
 * Returns true when there is a transmit opportunity (a complete message has been received)
 */

bool
Receiver::receive(Rs485 * rs485, sysState_t * sys)
{
	StaticJsonBuffer<CONFIG_POOL_JSON_BUFSIZE> jsonBuffer;
	JsonObject * root = &jsonBuffer.createObject();  // used for debug messages

	enum class state_t {
		_findPreamble,
		_readHeader,
		_readData,
		_readCRC,
		show
	};

	static pentairMsg_t msg;

	//static protocol_t proto;
	//static mHdr_a5_t hdr;
	//static uint8_t data[CONFIG_POOL_MSGDATA_BUFSIZE];
	//static uint16_t chk;
	static time_t start = _millis();  // Arduino: now()
	static state_t state = state_t::_findPreamble;
	bool txOpportunity = false;

	if (state != state_t::_findPreamble && _millis() - start > CONFIG_POOL_RS485_TIMEOUT) {
		printf("TIMEOUT\n");
		state = state_t::_findPreamble;
	}

	result_t rc = RESULT_error;  // init to please compiler
	switch (state) {
		case state_t::_findPreamble: rc = _findPreamble(rs485, &msg.proto);    break;
		case state_t::_readHeader:   rc = _readHeader(rs485, msg.proto, &msg.hdr); break;
		case state_t::_readData:     rc = _readData(rs485, &msg.hdr, msg.data);    break;
		case state_t::_readCRC:      rc = _readCRC(rs485, msg.proto, &msg.chk);    break;
		case state_t::show: break;  // dummy
	}

	switch (rc) {
		case RESULT_done:
			switch (state) {
				case state_t::_findPreamble:
					start = _millis();
					state = state_t::_readHeader;
					break;
				case state_t::_readHeader:
					state = state_t::_readData;
					break;
				case state_t::_readData:
					state = state_t::_readCRC;
					break;
				case state_t::_readCRC:
					if (_verifyCRC(&msg, root) == true) {
						Decode::decode(&msg, sys, root);  // go ahead
					} else {
						Utils::jsonRaw(&msg, root, "raw");
					}
					state = state_t::show;
					break;
				case state_t::show:
					break;  // dummy
			}
			break;
		case RESULT_incomplete:
			break;
		case RESULT_error:
			state = state_t::_findPreamble;
			break;
	}

	if (state == state_t::show) {
		Utils::jsonPrint(root, NULL, 0);
		state = state_t::_findPreamble;

		txOpportunity = (msg.proto == PROTOCOL_a5) &&
			(Utils::addrGroup(msg.hdr.src) == ADDRGROUP_ctrl) &&
			(Utils::addrGroup(msg.hdr.dst) == ADDRGROUP_all);
	}
	return txOpportunity;
}
