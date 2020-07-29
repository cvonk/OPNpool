/**
 * @brief Network layer: support enum/various to string
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>

#include "../utils/utils.h"
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
 * network_circuit_t
 **/

static const char * const _network_circuits[] = {
#define XX(num, name) #name,
  NETWORK_CIRCUIT_MAP(XX)
#undef XX
};

const char *  // 1-based
network_circuit_str(network_circuit_t const circuit)
{
    if (circuit == 0x85) {
        return "heatBoost";
    }
    return ELEM_AT(_network_circuits, circuit, hex8_str(circuit));
}

int
network_circuit_nr(char const * const circuit_str)
{
    ELEM_POS(_network_circuits, circuit_str);
}


/**
 * network_pump_mode_t
 **/

static const char * const _network_pump_modes[] = {
#define XX(num, name) #name,
  NETWORK_PUMP_MODE_MAP(XX)
#undef XX
};

const char *  // 1-based
network_pump_mode_str(network_pump_mode_t const pump_mode)
{
    return ELEM_AT(_network_pump_modes, pump_mode, hex8_str(pump_mode));
}

/**
 * network_heat_src_t
 **/

static const char * const _network_heat_srcs[] = {
#define XX(num, name) #name,
  NETWORK_HEAT_SRC_MAP(XX)
#undef XX
};

const char *  // 1-based
network_heat_src_str(network_heat_src_t const heat_src)
{
    return ELEM_AT(_network_heat_srcs, heat_src, hex8_str(heat_src));
}

int
network_heat_src_nr(char const * const heat_src_str)
{
    ELEM_POS(_network_heat_srcs, heat_src_str);
}

/**
 *
 **/

char const *
network_pump_prg_str(uint16_t const address)
{
	char const * s;
	switch (address) {
		case 0x2BF0: s = "?"; break;
		case 0x02E4: s = "pgm"; break;    // program GPM
		case 0x02C4: s = "rpm"; break;    // program RPM
		case 0x0321: s = "eprg"; break;  // select ext prog, 0x0000=P0, 0x0008=P1, 0x0010=P2,
										 //                  0x0080=P3, 0x0020=P4
		case 0x0327: s = "eRpm0"; break; // program ext program RPM0
		case 0x0328: s = "eRpm1"; break; // program ext program RPM1
		case 0x0329: s = "eRpm2"; break; // program ext program RPM2
		case 0x032A: s = "eRpm3"; break; // program ext program RPM3
		default: s = hex16_str(address);
	}
	return s;
}


str_value_name_pair_t _typ_ctrls[] = {
#define XX(num, name) { .typ = num, .str = #name },
 NETWORK_TYP_CTRL_MAP(XX)
#undef XX
};

const char *
network_typ_ctrl_str(network_typ_ctrl_t typ)
{
    str_value_name_pair_t const * pair = _typ_ctrls;
    for(uint ii = 0; ii < ARRAY_SIZE(_typ_ctrls); ii++, pair++) {
        if (pair->typ == typ) {
            return pair->str;
        }
    }
    return hex8_str(typ);
}

str_value_name_pair_t _typ_pumps[] = {
#define XX(num, name) { .typ = num, .str = #name },
  NETWORK_TYP_PUMP_MAP(XX)
#undef XX
};

const char *
network_typ_pump_str(network_typ_pump_t typ)
{
    str_value_name_pair_t const * pair = _typ_pumps;
    for(uint ii = 0; ii < ARRAY_SIZE(_typ_pumps); ii++, pair++) {
        if (pair->typ == typ) {
            return pair->str;
        }
    }
    return hex8_str(typ);
}

str_value_name_pair_t _typ_chlors[] = {
#define XX(num, name) { .typ = num, .str = #name },
  NETWORK_TYP_CHLOR_MAP(XX)
#undef XX
};

const char *
network_typ_chlor_str(network_typ_chlor_t typ)
{
    str_value_name_pair_t const * pair = _typ_chlors;
    for(uint ii = 0; ii < ARRAY_SIZE(_typ_chlors); ii++, pair++) {
        if (pair->typ == typ) {
            return pair->str;
        }
    }
    return hex8_str(typ);
}

/**
 * msg_typ_t
 **/

static const char * const _network_msg_typs[] = {
#define XX(num, name, typ, proto, prot_typ) #name,
  NETWORK_MSG_TYP_MAP(XX)
#undef XX
};

const char *
network_msg_typ_str(network_msg_typ_t const typ)
{
    return ELEM_AT(_network_msg_typs, typ, hex8_str(typ));
}

int
network_msg_typ_nr(char const * const msg_typ_str)
{
    ELEM_POS(_network_msg_typs, msg_typ_str);
}


