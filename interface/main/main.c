/**
 * @brief Clock Calendar using ESP32
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Sander and Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
**/

#include <stdio.h>
#include <sdkconfig.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <ota_update_task.h>
#include <wifi_connect.h>

#include <factory_reset_task.h>
#include "http_post_server.h"
#include "https_client_task.h"
#include "display_task.h"
#include "mqtt_task.h"
#include "ipc_msgs.h"

#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

static char const * const TAG = "main_app";

static void
_initNvsFlash(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void
_mac2devname(uint8_t const * const mac, char * const name, size_t name_len) {
	typedef struct {
		uint8_t const mac[WIFI_DEVMAC_LEN];
		char const * const name;
	} knownBrd_t;
	static knownBrd_t knownBrds[] = {
        { {0x30, 0xAE, 0xA4, 0x1A, 0x20, 0xF0}, "calclock-1" },
        { {0x30, 0xAE, 0xA4, 0x24, 0x2C, 0x98}, "calclock-2" },
        { {0x30, 0xae, 0xa4, 0xcc, 0x45, 0x04}, "esp32-wrover-1" },
        { {0x30, 0xAE, 0xA4, 0xCC, 0x42, 0x78}, "esp32-wrover-2" },
	};
	for (uint ii=0; ii < ARRAYSIZE(knownBrds); ii++) {
		if (memcmp(mac, knownBrds[ii].mac, WIFI_DEVMAC_LEN) == 0) {
			strncpy(name, knownBrds[ii].name, name_len);
			return;
		}
	}
	snprintf(name, name_len, "esp32_%02x%02x",
			 mac[WIFI_DEVMAC_LEN-2], mac[WIFI_DEVMAC_LEN-1]);
}

static void
_wifi_connect_cb(void * const priv_void, esp_ip4_addr_t const * const ip)
{
    ipc_t * const ipc = priv_void;
    ipc->dev.connectCnt.wifi++;
    snprintf(ipc->dev.ipAddr, WIFI_DEVIPADDR_LEN, IPSTR, IP2STR(ip));

    uint8_t mac[WIFI_DEVMAC_LEN];
    ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));
	_mac2devname(mac, ipc->dev.name, WIFI_DEVNAME_LEN);

    ESP_LOGI(TAG, "%s / %s / %u", ipc->dev.ipAddr, ipc->dev.name, ipc->dev.connectCnt.wifi);
}

void
app_main()
{
    _initNvsFlash();

    ESP_LOGI(TAG, "starting ..");
    xTaskCreate(&factory_reset_task, "factory_reset_task", 4096, NULL, 5, NULL);

    static ipc_t ipc;
    ipc.toClientQ = xQueueCreate(2, sizeof(toClientMsg_t));
    ipc.toDisplayQ = xQueueCreate(2, sizeof(toDisplayMsg_t));
    ipc.toMqttQ = xQueueCreate(2, sizeof(toMqttMsg_t));
    ipc.dev.connectCnt.wifi = 0;
    ipc.dev.connectCnt.mqtt = 0;
    assert(ipc.toDisplayQ && ipc.toClientQ && ipc.toMqttQ);

    wifi_connect_config_t wifi_connect_config = {
        .onConnect = _wifi_connect_cb,
        .priv = &ipc,
    };
    if (wifi_connect(&wifi_connect_config) != ESP_OK) {
        // should switch to factory partition for provisioning
    }

    // from here the tasks take over

    xTaskCreate(&ota_update_task, "ota_update_task", 4096, NULL, 5, NULL);
    xTaskCreate(&display_task, "display_task", 4096, &ipc, 5, NULL);
    xTaskCreate(&https_client_task, "https_client_task", 4096, &ipc, 5, NULL);
    xTaskCreate(&mqtt_task, "mqtt_task", 2*4096, &ipc, 5, NULL);
}