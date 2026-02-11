/**
 * @brief OPNpool - Network layer: creates network_msg from datalink_pkt and visa versa
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

#include "../datalink/datalink.h"
#include "network.h"

uint8_t
network_ic_len(uint8_t const ic_typ)
{
    switch (ic_typ) {
        case NETWORK_TYP_CHLOR_PING_REQ:
            return sizeof(network_msg_chlor_ping_req_t);
        case NETWORK_TYP_CHLOR_PING_RESP:
            return sizeof(network_msg_chlor_ping_resp_t);
        case NETWORK_TYP_CHLOR_NAME_RESP:
            return sizeof(network_msg_chlor_name_resp_t);
        case NETWORK_TYP_CHLOR_LEVEL_SET:
            return sizeof(network_msg_chlor_level_set_t);
        case NETWORK_TYP_CHLOR_LEVEL_RESP:
            return sizeof(network_msg_chlor_ping_resp_t);
        case NETWORK_TYP_CHLOR_NAME_REQ:
            return sizeof(network_msg_chlor_name_req_t);
        default:
            return 0;
    };
}
