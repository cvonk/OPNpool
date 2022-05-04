/**
 * @brief OPNpool - Data Link layer: bytes from the RS485 transceiver to/from data packets
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

#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "../rs485/rs485.h"
#include "skb/skb.h"
#include "datalink.h"

// static char const * const TAG = "datalink";

datalink_preamble_a5_t datalink_preamble_a5 = { 0x00, 0xFF, 0xA5 };  // use 0xA5 in the preamble to detection more reliable
datalink_preamble_ic_t datalink_preamble_ic = { 0x10, 0x02 };
datalink_preamble_ic_t datalink_postamble_ic = { 0x10, 0x03 };

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

uint16_t
datalink_calc_crc(uint8_t const * const start, uint8_t const * const stop)
{
    uint16_t crc = 0;
    for (uint8_t const * byte = start; byte < stop; byte++) {
        crc += *byte;
    }
    return crc;
}
