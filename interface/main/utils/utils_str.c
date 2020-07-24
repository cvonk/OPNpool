/**
 * @brief utils: convert various to string reusing a fixed buffer
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>

#include "utils_str.h"

name_str_t name_str = {
	.noMem = "sNOMEM", // increase size of str.str[]
	.digits = "0123456789ABCDEF"
};

void
name_reset_idx(void) {
	name_str.idx = 0;
}

char const *
hex8_str(uint8_t const value)
{
	uint_least8_t const nrdigits = sizeof(value) << 1;

	if (name_str.idx + nrdigits + 1U >= ARRAY_SIZE(name_str.str)) {
		return name_str.noMem;  // increase size of str.str[]
	}
	char * s = name_str.str + name_str.idx;
	s[0] = name_str.digits[(value & 0xF0) >> 4];
	s[1] = name_str.digits[(value & 0x0F)];
	s[nrdigits] = '\0';
	name_str.idx += nrdigits + 1U;
	return s;
}

char const *
hex16_str(uint16_t const value)
{
	uint_least8_t const nrdigits = sizeof(value) << 1;

	if (name_str.idx + nrdigits + 1U >= ARRAY_SIZE(name_str.str)) {
		return name_str.noMem;  // increase size of str.str[]
	}
	char * s = name_str.str + name_str.idx;
	s[0] = name_str.digits[(value & 0xF000) >> 12];
	s[1] = name_str.digits[(value & 0x0F00) >> 8];
	s[2] = name_str.digits[(value & 0x00F0) >> 4];
	s[3] = name_str.digits[(value & 0x000F) >> 0];
	s[nrdigits] = '\0';
	name_str.idx += nrdigits + 1U;
	return s;
}

#if 0
char const *
name_name(char const * const name, uint_least8_t const len)
{
	if (name_str.idx + len + 1U >= ARRAY_SIZE(name_str.str)) {
		return name_str.noMem;  // increase size of str.str[]
	}
	char * s = name_str.str + name_str.idx;

	for (uint_least8_t ii = 0; ii < len; ii++) {
		s[ii] = name[ii];
	}
	s[len] = '\0';
	name_str.idx += len + 1U;
	return s;
}
#endif
