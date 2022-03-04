/**
 * @brief Provision WiFi using BLE and phone app, then start OTA update
 **/
// Copyright © 2020-2022, Coert Vonk
// SPDX-License-Identifier: MIT
/*
   loosly based on: ...\espressif\esp-idf-v4.1-beta2\examples\provisioning\legacy\ble_prov\main\ble_main.c
   BLE based Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "wifi_connect.h"
#include "factory_ble_prov.h"
#include "ota_update_task.h"

/**
 * _ble_device_name_prefix and BLE_PROV_POP must match build.gradle in the android app;
 * SECURITY_VERSION must correspond to View > Tool Windows > Build Variants > app = bleSec?Debug in
 * Android Studio.
 */
#define BLE_PROV_SECURITY_VERSION (1)
#define BLE_PROV_POP ("bqeL7mTz")
#define BLE_PROV_RECONN_ATTEMPTS (3)
static char const * _ble_device_name_prefix = "POOL_";  // so we can use the Pool Configurator Android app, generic is "PROV_"

static char const * const TAG = "factory";

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
    static protocomm_security_pop_t const pop = {
        .data = (uint8_t const * const) BLE_PROV_POP,
        .len = (sizeof(BLE_PROV_POP)-1)
    };
    ESP_ERROR_CHECK(ble_prov_start_provisioning(_ble_device_name_prefix, BLE_PROV_SECURITY_VERSION, &pop));
}

static esp_err_t
_wifi_connect_cb(void * const priv_void, esp_ip4_addr_t const * const ip)
{
    ESP_LOGI(TAG, "IP addr " IPSTR, IP2STR(ip));
    _retry_num.ap_not_found = _retry_num.ap_auth_fail = 0;

    xTaskCreate(&ota_update_task, "ota_update_task", 8192, NULL, 5, NULL);
    return ESP_OK;
}

static esp_err_t
_wifi_disconnect_cb(void * const priv_void, bool const auth_err)
{
    if (auth_err) {
            ESP_LOGW(TAG, "WiFi auth error");
            if (_retry_num.ap_auth_fail < BLE_PROV_RECONN_ATTEMPTS) {
                _retry_num.ap_auth_fail++;
            } else {
                _start_provisioning();
                return ESP_FAIL;
            }
    } else {
            ESP_LOGW(TAG, "WiFi AP not found");
            if (_retry_num.ap_not_found < BLE_PROV_RECONN_ATTEMPTS) {
                _retry_num.ap_not_found++;
            } else {
                _start_provisioning();
                return ESP_FAIL;
            }
    }
    ESP_LOGI(TAG, "retry connect to WiFi ..");
    return ESP_OK;
}

void app_main(void)
{
    _init_nvs();

    wifi_connect_config_t wifi_connect_config = {
        .onConnect = _wifi_connect_cb,
        .onDisconnect = _wifi_disconnect_cb,
        .priv = NULL,
    };
    ESP_ERROR_CHECK(wifi_connect_init(&wifi_connect_config));

    bool provisioned;
    if (ble_prov_is_provisioned(&provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "Can't get device prov state");
        return;
    }
    if (provisioned) {
        // 2BD: for some reason `esp_wifi_start` fails with ESP_ERR_WIFI_CONN
        //   wifi_connect: WIFI_EVENT_STA_START
        //   E (52348) wifi:sta is connecting, return error
        //   ESP_ERROR_CHECK failed: esp_err_t 0x3007 (ESP_ERR_WIFI_CONN)
        // no despair, 'cause the device reboots, and then establishes WiFi connection
        ESP_ERROR_CHECK(wifi_connect_start(NULL));
    } else {
        _start_provisioning();
    }
}
