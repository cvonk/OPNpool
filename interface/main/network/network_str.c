/**
 * @brief Network layer: enum/various to string
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>

#include "../utils/utils_str.h"
//#include "../datalink/datalink.h"
#include "network.h"

char const *
network_date_str(uint8_t const year, uint8_t const month, uint8_t const day)
{
	uint_least8_t const nrdigits = 10; // 2015-12-31

	if (name_str.idx + nrdigits + 1U >= ARRAY_SIZE(name_str.str)) {
		return name_str.noMem;  // increase size of str.str[]
	}
	char * s = name_str.str + name_str.idx;
	s[0] = '2'; s[1] = '0';
	s[2] = name_str.digits[year / 10];
	s[3] = name_str.digits[year % 10];
	s[4] = s[7] = '-';
	s[5] = name_str.digits[month / 10];
	s[6] = name_str.digits[month % 10];
	s[8] = name_str.digits[day / 10];
	s[9] = name_str.digits[day % 10];
	s[nrdigits] = '\0';
	name_str.idx += nrdigits + 1U;
	return s;
}

char const *
network_time_str(uint8_t const hours, uint8_t const minutes)
{
	uint const nrdigits = 5;  // 00:00

	if (name_str.idx + nrdigits + 1U >= ARRAY_SIZE(name_str.str)) {
		return name_str.noMem;  // increase size of str.str[]
	}
	char * s = name_str.str + name_str.idx;
	s[0] = name_str.digits[hours / 10];
	s[1] = name_str.digits[hours % 10];
	s[2] = ':';
	s[3] = name_str.digits[minutes / 10];
	s[4] = name_str.digits[minutes % 10];
	s[nrdigits] = '\0';
	name_str.idx += nrdigits + 1U;
	return s;
}

char const *
network_version_str(uint8_t const major, uint8_t const minor)
{
	uint const nrdigits = 5;  // 2.8

	if (name_str.idx + nrdigits + 1U >= ARRAY_SIZE(name_str.str)) {
		return name_str.noMem;  // increase size of str.str[]
	}
	char * s = name_str.str + name_str.idx;
	s[0] = name_str.digits[major / 10];
	s[1] = name_str.digits[major % 10];
	s[2] = '.';
	s[3] = name_str.digits[minor / 10];
	s[4] = name_str.digits[minor % 10];
	s[nrdigits] = '\0';
	name_str.idx += nrdigits + 1U;
	return s;
}

/**
 * enum to string
 */

static char const * const _chlor_states[] = {
	"ok", "highSalt", "lowSalt", "veryLowSalt", "lowFlow"
};

char const *
name_chlor_state(uint8_t const chlorstate)
{
	if (chlorstate < ARRAY_SIZE(_chlor_states)) {
		return _chlor_states[chlorstate];
	}
	return hex8_str(chlorstate);
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
	return hex8_str(value);
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
		default: s = hex16_str(address);
	}
	return s;
}

static char const * const _circuits[] = {
	"spa", "aux1", "aux2", "aux3", "ft1", "pool", "ft1", "ft2", "ft3", "ft4"
};

char const *
name_circuit(uint8_t const circuit)
{
	if (circuit && circuit <= ARRAY_SIZE(_circuits)) {
		return _circuits[circuit - 1];
	}
	if (circuit == 0x85) {
		return "heatBoost";
	}
	return hex8_str(circuit);
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
	return hex8_str(value);
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

static const char * const _msgtype_names[] = {
#define XX(num, name) #name,
  NETWORK_MSGTYP_MAP(XX)
#undef XX
};

const char *
name_network_msgtyp(NETWORK_MSGTYP_t typ)
{
  return ELEM_AT(_msgtype_names, typ, hex8_str(typ));
}

