/**
 * @brief OPNpool - utils: return hardcoded board name based on MAC address
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
 */

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_wifi.h>

#include "utils.h"

#define WIFI_DEVMAC_LEN (6)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

typedef struct {
    uint8_t const mac[WIFI_DEVMAC_LEN];
    char const * const name;
} knownBrd_t;

static knownBrd_t _knownBrds[] = {
    { {0xb4, 0xe6, 0x2d, 0x96, 0x53, 0x71}, "opnpool" },
    { {0x30, 0xae, 0xa4, 0xcc, 0x45, 0x04}, "esp32-wrover-1" },
    { {0x30, 0xAE, 0xA4, 0xCC, 0x42, 0x78}, "esp32-wrover-2" },
};

void
board_name(char * const name, size_t name_len)
{
    uint8_t mac[WIFI_DEVMAC_LEN];
    ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));

	for (uint ii=0; ii < ARRAY_SIZE(_knownBrds); ii++) {
		if (memcmp(mac, _knownBrds[ii].mac, WIFI_DEVMAC_LEN) == 0) {
			strncpy(name, _knownBrds[ii].name, name_len);
            name[name_len - 1] = '\0';
			return;
		}
	}
	snprintf(name, name_len, "esp32_%02x%02x",
			 mac[WIFI_DEVMAC_LEN-2], mac[WIFI_DEVMAC_LEN-1]);
}
