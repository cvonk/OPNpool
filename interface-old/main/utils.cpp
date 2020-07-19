/**
 * Various support function for transmitting/receiving over the Pentair RS-485 bus
 */

#include <stdio.h>
#include "ArduinoJson.h"
#include <sdkconfig.h>
#include "pooltypes.h"
#include "utils.h"

// reusable global string
struct str_t {
	char str[K_STR_BUF_SIZE];  // 3 bytes for each hex value when displaying raw; this is str.str[]
	uint_least8_t idx;
	char const * const noMem;
	char const * const digits;
} g_str = {
	{},
	0,
	"sNOMEM", // increase size of str.str[]
	"0123456789ABCDEF"
};

void
Utils::resetIdx(void) {
	g_str.idx = 0;
}

char const *
Utils::strHex16(uint16_t const value)
{
	uint_least8_t const nrdigits = sizeof(value) << 1;

	if (g_str.idx + nrdigits + 1U >= ARRAY_SIZE(g_str.str)) {
		return g_str.noMem;  // increase size of str.str[]
	}
	char * s = g_str.str + g_str.idx;
	s[0] = g_str.digits[(value & 0xF000) >> 12];
	s[1] = g_str.digits[(value & 0x0F00) >> 8];
	s[2] = g_str.digits[(value & 0x00F0) >> 4];
	s[3] = g_str.digits[(value & 0x000F) >> 0];
	s[nrdigits] = '\0';
	g_str.idx += nrdigits + 1U;
	return s;
}

char const *
Utils::strDate(uint8_t const year, uint8_t const month, uint8_t const day)
{
	uint_least8_t const nrdigits = 10; // 2015-12-31

	if (g_str.idx + nrdigits + 1U >= ARRAY_SIZE(g_str.str)) {
		return g_str.noMem;  // increase size of str.str[]
	}
	char * s = g_str.str + g_str.idx;
	s[0] = '2'; s[1] = '0';
	s[2] = g_str.digits[year / 10];
	s[3] = g_str.digits[year % 10];
	s[4] = s[7] = '-';
	s[5] = g_str.digits[month / 10];
	s[6] = g_str.digits[month % 10];
	s[8] = g_str.digits[day / 10];
	s[9] = g_str.digits[day % 10];
	s[nrdigits] = '\0';
	g_str.idx += nrdigits + 1U;
	return s;
}

char const *
Utils::strTime(uint8_t const hours, uint8_t const minutes)
{
	uint_least8_t const nrdigits = 5;  // 00:00

	if (g_str.idx + nrdigits + 1U >= ARRAY_SIZE(g_str.str)) {
		return g_str.noMem;  // increase size of str.str[]
	}
	char * s = g_str.str + g_str.idx;
	s[0] = g_str.digits[hours / 10];
	s[1] = g_str.digits[hours % 10];
	s[2] = ':';
	s[3] = g_str.digits[minutes / 10];
	s[4] = g_str.digits[minutes % 10];
	s[nrdigits] = '\0';
	g_str.idx += nrdigits + 1U;
	return s;
}

/**
 * convert enum to string
 */

char const *
Utils::mtCtrlName(MT_CTRL_a5_t const mt, bool * const found)
{
	char const * s = NULL;
	switch (mt) {
		case MT_CTRL_setAck:     s = "setAck";     break;
		case MT_CTRL_circuitSet: s = "circuitSet"; break;
		case MT_CTRL_state:      s = "state";      break;
		case MT_CTRL_stateSet:   s = "stateSet";   break;
		case MT_CTRL_stateReq:   s = "stateReq";   break;
		case MT_CTRL_time:       s = "time";       break;
		case MT_CTRL_timeSet:    s = "timeSet";    break;
		case MT_CTRL_timeReq:    s = "timeReq";    break;
		case MT_CTRL_heat:       s = "heat";       break;
		case MT_CTRL_heatSet:    s = "heatSet";    break;
		case MT_CTRL_heatReq:    s = "heatReq";    break;
		case MT_CTRL_sched:      s = "sched";      break;
			// case MT_CTRL_schedSet:   s = "schedSet";   break;
		case MT_CTRL_schedReq:   s = "schedReq";   break;
#if 0
		case MT_CTRL_layout:     s = "layout";     break;
		case MT_CTRL_layoutSet:  s = "layoutSet";  break;
		case MT_CTRL_layoutReq:  s = "layoutReq";  break;
#endif
	}
	if (found) {
		*found = (s != NULL);
	}
	return s;
}

char const *
Utils::mtPumpName(MT_PUMP_a5_t const mt, bool const request, bool * const found)
{
	char const * s = NULL;
	char const * const ff = "FF";
	if (request) {
		switch (mt) {
			case MT_PUMP_regulate: s = "setReguReq";  break;
			case MT_PUMP_control:  s = "setCtrlReq";  break;
			case MT_PUMP_mode:     s = "setModeReq";  break;
			case MT_PUMP_state:    s = "setStateReq"; break;
			case MT_PUMP_status:   s = "statusReq";   break;
			case MT_PUMP_0xFF:     s = ff;            break;
		}
	}
	else {
		switch (mt) {
			case MT_PUMP_regulate: s = "setReguResp";  break;
			case MT_PUMP_control:  s = "setCtrlResp";  break;
			case MT_PUMP_mode:     s = "setModeResp";  break;
			case MT_PUMP_state:    s = "setStateResp"; break;
			case MT_PUMP_status:   s = "status";       break;
			case MT_PUMP_0xFF:     s = ff;             break;
		}
	}
	if (found) {
		*found = (s != NULL);
	}
	return s;
}

char const *
Utils::mtChlorName(MT_CHLOR_ic_t const mt, bool * const found)
{
	char const * s = NULL;
	switch (mt) {
		case MT_CHLOR_pingReq:    s = "pingReq";    break;
		case MT_CHLOR_ping:       s = "ping";       break;
		case MT_CHLOR_name:       s = "name";       break;
		case MT_CHLOR_lvlSet:     s = "lvlSet";     break;
		case MT_CHLOR_lvlSetResp: s = "lvlSetResp"; break;
		case MT_CHLOR_0x14:       s = "14";         break;
	}
	if (found) {
		*found = (s != NULL);
	}
	return s;
}


char const *
Utils::strHex8(uint8_t const value)
{
	uint_least8_t const nrdigits = sizeof(value) << 1;

	if (g_str.idx + nrdigits + 1U >= ARRAY_SIZE(g_str.str)) {
		return g_str.noMem;  // increase size of str.str[]
	}
	char * s = g_str.str + g_str.idx;
	s[0] = g_str.digits[(value & 0xF0) >> 4];
	s[1] = g_str.digits[(value & 0x0F)];
	s[nrdigits] = '\0';
	g_str.idx += nrdigits + 1U;
	return s;
}


char const *
Utils::strName(char const * const name, uint_least8_t const len)
{
	if (g_str.idx + len + 1U >= ARRAY_SIZE(g_str.str)) {
		return g_str.noMem;  // increase size of str.str[]
	}
	char * s = g_str.str + g_str.idx;

	for (uint_least8_t ii = 0; ii < len; ii++) {
		s[ii] = name[ii];
	}
	s[len] = '\0';
	g_str.idx += len + 1U;
	return s;
}

char const * const kCircuitNames[] = {
	"spa", "aux1", "aux2", "aux3", "ft1", "pool", "ft1", "ft2", "ft3", "ft4"
};

char const * 
Utils::circuitName(uint8_t const circuit)
{
	if (circuit > 0 && circuit <= ARRAY_SIZE(kCircuitNames)) {
		return kCircuitNames[circuit - 1];
	}
	if (circuit == 0x85) {
		return "heatBoost";
	}
	return strHex8(circuit);
}

char const * const kChlorStateNames[] = {
	"ok", "highSalt", "lowSalt", "veryLowSalt", "lowFlow"
};

char const * 
Utils::chlorStateName(uint8_t const chlorstate)
{
	if (chlorstate < ARRAY_SIZE(kChlorStateNames)) {
		return kChlorStateNames[chlorstate];
	}
	return strHex8(chlorstate);
}

uint_least8_t   // 1-based
Utils::circuitNr(char const * const name)
{
	for (uint_least8_t ii = 0; ii < ARRAY_SIZE(kCircuitNames); ii++) {
		if (strcmp(name, kCircuitNames[ii]) == 0) {
			return ii + 1;
		}
	}
	return 0;
}

char const * const kHeatSrcNames[] = {
	"none", "heater", "solarPref", "solar"
};

char const * 
Utils::strHeatSrc(uint8_t const value)
{
	if (value < ARRAY_SIZE(kHeatSrcNames)) {
		return kHeatSrcNames[value];
	}
	return strHex8(value);
}

uint_least8_t 
Utils::heatSrcNr(char const * const name)
{
	for (uint_least8_t ii = 0; ii < ARRAY_SIZE(kHeatSrcNames); ii++) {
		if (strcmp(name, kHeatSrcNames[ii]) == 0) {
			return ii;
		}
	}
	return 0;
}

char const * const kPumpModeNames[] = {
	"filter", "man", "bkwash", "3", "4", "5",
	"ft1", "7", "8", "ep1", "ep2", "ep3", "ep4"
};

char const * 
Utils::strPumpMode(uint16_t const value)
{
	if (value < ARRAY_SIZE(kPumpModeNames)) {
		return kPumpModeNames[value];
	}
	return strHex8(value);
}

char const * 
Utils::strPumpPrgName(uint16_t const address)
{
	char const * s;
	switch (address) {
		case 0x2BF0: s = "?"; break;
		case 0x02E4: s = "pgm"; break;    // program GPM
		case 0x02C4: s = "rpm"; break;    // program RPM
#if 0
		case 0x0321: s = "eprg"; break;  // select ext prog, 0x0000=P0, 0x0008=P1, 0x0010=P2,
										 //                  0x0080=P3, 0x0020=P4
		case 0x0327: s = "eRpm0"; break; // program ext program RPM0
		case 0x0328: s = "eRpm1"; break; // program ext program RPM1
		case 0x0329: s = "eRpm2"; break; // program ext program RPM2
		case 0x032A: s = "eRpm3"; break; // program ext program RPM3
#endif
		default: s = Utils::strHex16(address);
	}
	return s;
}

/**
 * address functions
 */

addrGroup_t
Utils::addrGroup(uint_least8_t const addr)
{
	return (addrGroup_t)(addr >> 4);
}

uint8_t 
Utils::addr(uint8_t group, uint8_t const id)
{
	return (group << 4) | id;
}

void
Utils::activeCircuits(JsonObject & json, char const * const key, uint16_t const value)
{
	JsonArray & active = json.createNestedArray(key);

	uint16_t mask = 0x00001;
	for (uint_least8_t ii = 0; mask; ii++) {
		if (value & mask) {
            active.add(Utils::circuitName(ii + 1));
		}
		mask <<= 1;
	}
}

bool
Utils::circuitIsActive(char const * const key, uint16_t const value)
{
	uint16_t mask = 0x00001;
	for (uint_least8_t ii = 0; mask; ii++) {
		if (strcmp(key, Utils::circuitName(ii + 1)) == 0) {
			return value & mask;
		}
		mask <<= 1;
	}
	return false;
}

void
Utils::schedule(JsonObject & json, uint8_t const circuit, uint16_t const start, uint16_t const stop)
{
	char const * const key = Utils::circuitName(circuit);
	JsonObject & sched = json.createNestedObject(key);
	sched["start"] = Utils::strTime(start / 60, start % 60);
	sched["stop"] = Utils::strTime(stop / 60, stop % 60);
}

//#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void
Utils::jsonRaw(pentairMsg_t * msg, JsonObject * json, char const * const key)
{
	if (json  && CONFIG_POOL_DBG_RAW_ONERROR) {
		{
			//JsonObject & sub = json->createNestedObject((proto == PROTOCOL_a5) ? "a5" : "ic");
		} {
			JsonObject * obj = &json->createNestedObject(key);
			uint8_t const * const raw = (uint8_t *)&msg->hdr;
			JsonArray & array = obj->createNestedArray("header");
			for (uint_least8_t ii = 0; ii < sizeof(msg->hdr); ii++) {
				array.add(Utils::strHex8(raw[ii]));
			}
		} {
			JsonArray & array = json->createNestedArray("data");
			for (uint_least8_t ii = 0; ii < msg->hdr.len; ii++) {
				array.add(Utils::strHex8(msg->data[ii]));
			}
		}
	}
}

size_t
Utils::jsonPrint(JsonObject * json, char * buffer, size_t bufferSize)
{
	if (json) {
		if (json->success()) {
			if (json->size() != 0) {
				if (buffer) {
					json->printTo(buffer, bufferSize);
				}
				else {
					char buf[300];
					json->printTo(buf, sizeof(buf)-1);
					printf("%s\n", buf);
				}
			}
		}
		else {
			printf("jNOMEM\n");  // increase size of StaticJsonBuffer
		}
	}
	return 0;
}

static uint8_t preamble_a5[] = { 0x00, 0xFF, 0xA5 };
static uint8_t preamble_ic[] = { 0x10, 0x02 };

uint16_t
Utils::calcCRC_a5(mHdr_a5_t const * const hdr, uint8_t const * const data)
{
	// checksum is calculated starting at the last byte of the preamble (0xA5) up to the last
	// data byte
	uint16_t calc = preamble_a5[sizeof(preamble_a5) - 1];
	for (uint_least8_t ii = 0; ii < sizeof(mHdr_a5_t); ii++) {
		calc += ((uint8_t *)hdr)[ii];
	}
	if (data) {
		for (uint_least8_t ii = 0; ii < hdr->len; ii++) {
			calc += data[ii];
		}
	}
	return calc;
}

uint8_t
Utils::calcCRC_ic(mHdr_a5_t const * const hdr, uint8_t const * const data)
{
	// checksum is calculated starting at the preamble up to the last data byte
	uint8_t calc = hdr->dst + hdr->typ;  // pretend we still have the ic header
	for (uint_least8_t ii = 0; ii < sizeof(preamble_ic); ii++) {
		calc += preamble_ic[ii];
	}
	for (uint_least8_t ii = 0; ii < hdr->len; ii++) {
		calc += data[ii];
	}
	return calc;
}
