/**
 * @brief Pool state to string
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>

#include "name.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

// also used to display date/time, add +50
#define NAME_BUF_SIZE ((sizeof(mHdr_a5_t) + sizeof(mCtrlState_a5_t) + 1) * 3 + 50)

// reusable global string
static struct str_t {
	char str[NAME_BUF_SIZE];  // 3 bytes for each hex value when displaying raw; this is str.str[]
	uint_least8_t idx;
	char const * const noMem;
	char const * const digits;
} _str = {
	{},
	0,
	"sNOMEM", // increase size of str.str[]
	"0123456789ABCDEF"
};

void
name_reset_idx(void) {
	_str.idx = 0;
}

char const *
name_hex8(uint8_t const value)
{
	uint_least8_t const nrdigits = sizeof(value) << 1;

	if (_str.idx + nrdigits + 1U >= ARRAY_SIZE(_str.str)) {
		return _str.noMem;  // increase size of str.str[]
	}
	char * s = _str.str + _str.idx;
	s[0] = _str.digits[(value & 0xF0) >> 4];
	s[1] = _str.digits[(value & 0x0F)];
	s[nrdigits] = '\0';
	_str.idx += nrdigits + 1U;
	return s;
}

char const *
name_hex16(uint16_t const value)
{
	uint_least8_t const nrdigits = sizeof(value) << 1;

	if (_str.idx + nrdigits + 1U >= ARRAY_SIZE(_str.str)) {
		return _str.noMem;  // increase size of str.str[]
	}
	char * s = _str.str + _str.idx;
	s[0] = _str.digits[(value & 0xF000) >> 12];
	s[1] = _str.digits[(value & 0x0F00) >> 8];
	s[2] = _str.digits[(value & 0x00F0) >> 4];
	s[3] = _str.digits[(value & 0x000F) >> 0];
	s[nrdigits] = '\0';
	_str.idx += nrdigits + 1U;
	return s;
}

char const *
name_name(char const * const name, uint_least8_t const len)
{
	if (_str.idx + len + 1U >= ARRAY_SIZE(_str.str)) {
		return _str.noMem;  // increase size of str.str[]
	}
	char * s = _str.str + _str.idx;

	for (uint_least8_t ii = 0; ii < len; ii++) {
		s[ii] = name[ii];
	}
	s[len] = '\0';
	_str.idx += len + 1U;
	return s;
}

char const *
name_date(uint8_t const year, uint8_t const month, uint8_t const day)
{
	uint_least8_t const nrdigits = 10; // 2015-12-31

	if (_str.idx + nrdigits + 1U >= ARRAY_SIZE(_str.str)) {
		return _str.noMem;  // increase size of str.str[]
	}
	char * s = _str.str + _str.idx;
	s[0] = '2'; s[1] = '0';
	s[2] = _str.digits[year / 10];
	s[3] = _str.digits[year % 10];
	s[4] = s[7] = '-';
	s[5] = _str.digits[month / 10];
	s[6] = _str.digits[month % 10];
	s[8] = _str.digits[day / 10];
	s[9] = _str.digits[day % 10];
	s[nrdigits] = '\0';
	_str.idx += nrdigits + 1U;
	return s;
}

char const *
name_time(uint8_t const hours, uint8_t const minutes)
{
	uint_least8_t const nrdigits = 5;  // 00:00

	if (_str.idx + nrdigits + 1U >= ARRAY_SIZE(_str.str)) {
		return _str.noMem;  // increase size of str.str[]
	}
	char * s = _str.str + _str.idx;
	s[0] = _str.digits[hours / 10];
	s[1] = _str.digits[hours % 10];
	s[2] = ':';
	s[3] = _str.digits[minutes / 10];
	s[4] = _str.digits[minutes % 10];
	s[nrdigits] = '\0';
	_str.idx += nrdigits + 1U;
	return s;
}

/**
 * enum to string
 */

char const *
name_mtCtrl(MT_CTRL_A5_t const mt, bool * const found)
{
	char const * s = NULL;
	switch (mt) {
		case MT_CTRL_SET_ACK:     s = "setAck";     break;
		case MT_CTRL_CIRCUIT_SET: s = "circuitSet"; break;
		case MT_CTRL_STATE:       s = "state";      break;
		case MT_CTRL_STATE_SET:   s = "stateSet";   break;
		case MT_CTRL_STATE_REQ:   s = "stateReq";   break;
		case MT_CTRL_TIME:        s = "time";       break;
		case MT_CTRL_TIME_SET:    s = "timeSet";    break;
		case MT_CTRL_TIME_REQ:    s = "timeReq";    break;
		case MT_CTRL_HEAT:        s = "heat";       break;
		case MT_CTRL_HEAT_SET:    s = "heatSet";    break;
		case MT_CTRL_HEAT_REQ:    s = "heatReq";    break;
		case MT_CTRL_SCHED:       s = "sched";      break;
			// case MT_CTRL_SCHED_SET:   s = "schedSet";   break;
		case MT_CTRL_SCHED_REQ:   s = "schedReq";   break;
#if 0
		case MT_CTRL_layout:      s = "layout";     break;
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
name_mtPump(MT_PUMP_A5_t const mt, bool const request, bool * const found)
{
	char const * s = NULL;
	char const * const ff = "FF";
	if (request) {
		switch (mt) {
			case MT_PUMP_REGULATE: s = "setReguReq";  break;
			case MT_PUMP_CTRL:     s = "setCtrlReq";  break;
			case MT_PUMP_MODE:     s = "setModeReq";  break;
			case MT_PUMP_STATE:    s = "setStateReq"; break;
			case MT_PUMP_STATUS:   s = "statusReq";   break;
			case MT_PUMP_0xFF:     s = ff;            break;
		}
	} else {
		switch (mt) {
			case MT_PUMP_REGULATE: s = "setReguResp";  break;
			case MT_PUMP_CTRL:     s = "setCtrlResp";  break;
			case MT_PUMP_MODE:     s = "setModeResp";  break;
			case MT_PUMP_STATE:    s = "setStateResp"; break;
			case MT_PUMP_STATUS:   s = "status";       break;
			case MT_PUMP_0xFF:     s = ff;             break;
		}
	}
	if (found) {
		*found = (s != NULL);
	}
	return s;
}

char const *
name_mtChlor(MT_CHLOR_IC_t const mt, bool * const found)
{
	char const * s = NULL;
	switch (mt) {
		case MT_CHLOR_PING_REQ:    s = "pingReq";    break;
		case MT_CHLOR_PING:        s = "ping";       break;
		case MT_CHLOR_NAME:        s = "name";       break;
		case MT_CHLOR_LVLSET:      s = "lvlSet";     break;
		case MT_CHLOR_LVLSET_RESP: s = "lvlSetResp"; break;
		case MT_CHLOR_0x14:        s = "14";         break;
	}
	if (found) {
		*found = (s != NULL);
	}
	return s;
}

static char const * const _chlor_states[] = {
	"ok", "highSalt", "lowSalt", "veryLowSalt", "lowFlow"
};

char const *
name_chlor_state(uint8_t const chlorstate)
{
	if (chlorstate < ARRAY_SIZE(_chlor_states)) {
		return _chlor_states[chlorstate];
	}
	return name_hex8(chlorstate);
}

static char const * const _pump_modes[] = {
	"filter", "man", "bkwash", "3", "4", "5",
	"ft1", "7", "8", "ep1", "ep2", "ep3", "ep4"
};

char const *
name_pump_mode(uint16_t const value)
{
	if (value < ARRAY_SIZE(_pump_modes)) {
		return _pump_modes[value];
	}
	return name_hex8(value);
}

char const *
name_pump_prg(uint16_t const address)
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
		default: s = name_hex16(address);
	}
	return s;
}

static char const * const _circuits[] = {
	"spa", "aux1", "aux2", "aux3", "ft1", "pool", "ft1", "ft2", "ft3", "ft4"
};

char const *
name_circuit(uint const circuit)
{
	if (circuit && circuit <= ARRAY_SIZE(_circuits)) {
		return _circuits[circuit - 1];
	}
	if (circuit == 0x85) {
		return "heatBoost";
	}
	return name_hex8(circuit);
}

uint   // 1-based
name_circuit_nr(char const * const name)
{
	for (uint ii = 0; ii < ARRAY_SIZE(_circuits); ii++) {
		if (strcmp(name, _circuits[ii]) == 0) {
			return ii + 1;
		}
	}
	return 0;
}

static char const * const _heat_srcs[] = {
	"none", "heater", "solarPref", "solar"
};

char const *
name_heat_src(uint8_t const value)
{
	if (value < ARRAY_SIZE(_heat_srcs)) {
		return _heat_srcs[value];
	}
	return name_hex8(value);
}

uint
name_heat_src_nr(char const * const name)
{
	for (uint_least8_t ii = 0; ii < ARRAY_SIZE(_heat_srcs); ii++) {
		if (strcmp(name, _heat_srcs[ii]) == 0) {
			return ii;
		}
	}
	return 0;
}
