/**
 * @brief OPNpool - convert various to string reusing a fixed buffer (from an earlier era)
 *
 * © Copyright 2014, 2019, 2022, Coert Vonk
 * 
 * This file is part of OPNpool.
 * OPNpool is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * OPNpool is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with OPNpool. 
 * If not, see <https://www.gnu.org/licenses/>.
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: Copyright 2014,2019,2022 Coert Vonk
 */

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>

#include "utils.h"

name_str_t name_str = {
	.noMem = "sNOMEM", // increase size of str.str[]
	.digits = "0123456789ABCDEF"
};

void
name_reset_idx(void) {
	name_str.idx = 0;
}

char const *
uint_str(uint16_t const value)
{
	size_t const nrdigits = 5;
	if (name_str.idx + nrdigits + 1U >= ARRAY_SIZE(name_str.str)) {
		return name_str.noMem;  // increase size of str.str[]
	}
	char * s = name_str.str + name_str.idx;

    size_t len = 0;
    uint16_t mask = 10000U;
    for (uint ii = 0; ii < nrdigits; ii++) {
        uint8_t const digit = value / mask;
        if (digit || ii == nrdigits - 1) {  // no leading 0s
          s[len++] = name_str.digits[digit];
        }
        mask /= 10;
    }
	s[len++] = '\0';
	name_str.idx += len;
	return s;
}

char const *
bool_str(bool const value)
{
    char const * const value_str = value ? "true" : "false";
    uint len = strlen(value_str);
	char * s = name_str.str + name_str.idx;
    strcpy(s, value_str);
	s[len++] = '\0';
	name_str.idx += len;
	return s;
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

