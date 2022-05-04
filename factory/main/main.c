/**
 * @brief Provision OPNpool using a phone app over BLE
 *  
 * Remember to clear the flash memory first.
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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "wifi_connect.h"
#include "factory_ble_prov.h"
#include "ota_update_task.h"
#include "factory_reset_task.h"

#if 0
#define FACTORY_BLE_PROV_RECONN_ATTEMPTS (3)
#endif

/**
 * _ble_device_name_prefix and BLE_PROV_POP must match build.gradle in the android app;
 * SECURITY_VERSION must correspond to View > Tool Windows > Build Variants > app = bleSec?Debug in
 * Android Studio.
 */
#define BLE_PROV_SECURITY_VERSION (1)
#define BLE_PROV_POP ("bqeL7mTz")
static char const * _ble_device_name_prefix = "POOL_";  // so we can use the Pool Configurator Android app, generic is "PROV_"

static char const * const TAG = "factory";

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t _wifi_event_group;

static struct retry_num {
    uint ap_not_found;
    uint ap_auth_fail;
} _retry_num = {};

static void
_init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
}

static void
_start_provisioning(void)
{
    ESP_LOGI(TAG, "Starting BLE provisioning");
#if 0
    static protocomm_security_pop_t const pop = {
        .data = (uint8_t const * const) BLE_PROV_POP,
        .len = (sizeof(BLE_PROV_POP)-1)
    };
#endif
    ESP_ERROR_CHECK(ble_prov_start_provisioning(_ble_device_name_prefix, BLE_PROV_SECURITY_VERSION, BLE_PROV_POP));
}

static esp_err_t
_wifi_connect_cb(void * const priv_void, esp_ip4_addr_t const * const ip)
{
    ESP_LOGI(TAG, "IP addr " IPSTR, IP2STR(ip));
    _retry_num.ap_not_found = _retry_num.ap_auth_fail = 0;

    // signal main() that we're connected
    xEventGroupSetBits(_wifi_event_group, WIFI_CONNECTED_EVENT);
    return ESP_OK;
}

static esp_err_t
_wifi_disconnect_cb(void * const priv_void, bool const auth_err)
{
    // signal main() that we're disconnected
    xEventGroupClearBits(_wifi_event_group, WIFI_CONNECTED_EVENT);

#if 0
    if (auth_err) {
            ESP_LOGW(TAG, "WiFi auth error");
            if (_retry_num.ap_auth_fail < FACTORY_BLE_PROV_RECONN_ATTEMPTS) {
                _retry_num.ap_auth_fail++;
            } else {
                _start_provisioning();
                return ESP_FAIL;
            }
    } else {
            ESP_LOGW(TAG, "WiFi AP not found");
            if (_retry_num.ap_not_found < FACTORY_BLE_PROV_RECONN_ATTEMPTS) {
                _retry_num.ap_not_found++;
            } else {
                _start_provisioning();
                return ESP_FAIL;
            }
    }
#endif    
    ESP_LOGI(TAG, "retrying connect to WiFi ..");
    return ESP_OK;
}

void app_main(void)
{
    _init_nvs();
    xTaskCreate(&factory_reset_task, "factory_reset_task", 4096, NULL, 5, NULL);

    _wifi_event_group = xEventGroupCreate();

    wifi_connect_config_t wifi_connect_config = {
        .onConnect = _wifi_connect_cb,
        .onDisconnect = _wifi_disconnect_cb,
        .priv = NULL,
    };
    ESP_LOGW(TAG, "Calling wifi_connect_init() ..");
    ESP_ERROR_CHECK(wifi_connect_init(&wifi_connect_config));

    ESP_LOGW(TAG, "Calling ble_prov_is_provisioned() ..");
    bool provisioned;
    if (ble_prov_is_provisioned(&provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "Can't get device prov state");
        return;
    }
    ESP_LOGW(TAG, "Provisioned = %s", provisioned ? "true" : "false");

    if (provisioned) {
        // 2BD: for some reason `esp_wifi_start` fails with ESP_ERR_WIFI_CONN
        //   wifi_connect: WIFI_EVENT_STA_START
        //   E (52348) wifi:sta is connecting, return error
        //   ESP_ERROR_CHECK failed: esp_err_t 0x3007 (ESP_ERR_WIFI_CONN)
        // no despair, 'cause the device reboots, and then establishes WiFi connection
        ESP_LOGW(TAG, "Calling wifi_connect_start() ..");
        ESP_ERROR_CHECK(wifi_connect_start(NULL));
    } else {
        ESP_LOGW(TAG, "Calling _start_provisioning() ..");
        _start_provisioning();
        ESP_LOGW(TAG, "Returned from _start_provisioning() ..");
    }

    // wait for WiFi connection
    xEventGroupWaitBits(_wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);

    xTaskCreate(&ota_update_task, "ota_update_task", 8192, "factory", 5, NULL);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
