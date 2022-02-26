/**
 * @brief Utils: returns hardcoded board name based on MAC address
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020-2022, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

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
    { {0xb4, 0xe6, 0x2d, 0x96, 0x53, 0x71}, "pool" },
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
