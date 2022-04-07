/**
 * @brief Interface with Android/iOS app to provision WiFi
 *
 * Written in 2019, 2022 by Coert Vonk 
 * 
 * To the extent possible under law, the author(s) have dedicated all copyright and related and
 * neighboring rights to this software to the public domain worldwide. This software is
 * distributed without any warranty. You should have received a copy of the CC0 Public Domain
 * Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 * 
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "factory_ble_prov.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "factory_ble_prov";

/* Signal Wi-Fi events on this event-group */
const int WIFI_PROV_END_EVENT = BIT0;
static EventGroupHandle_t _prov_event_group;

esp_err_t
ble_prov_is_provisioned(bool *provisioned)
{
    *provisioned = false;

    wifi_config_t wifi_cfg;
    if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg) != ESP_OK) {
        return ESP_FAIL;
    }
    if (strlen((const char*) wifi_cfg.sta.ssid)) {
        *provisioned = true;
        ESP_LOGI(TAG, "Found SSID %s", (const char*) wifi_cfg.sta.ssid);
        //ESP_LOGI(TAG, "Found password %s", (const char*) wifi_cfg.sta.password);
    }
    return ESP_OK;
}

/* Event handler for catching system events */
static void prov_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch (event_id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received SSID (%s), passwd (%s)",
                     (const char *) wifi_sta_cfg->ssid,
                     (const char *) wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s" "\n\tPlease retry provisioning",
                        (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                        "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");

            wifi_prov_mgr_reset_sm_state_on_failure();
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Wi-Fi provisioning successful");
            break;

        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Wi-Fi Provisioning ended");
            wifi_prov_mgr_stop_provisioning();
            wifi_prov_mgr_deinit();  // release resources once provisioning is finished
            xEventGroupSetBits(_prov_event_group, WIFI_PROV_END_EVENT);
            break;
        default:
            break;
    }
}

static void _get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "POOL_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

   /* Handlers for the optional provisioning endpoint registered by the application.
    * The data format can be chosen by applications. Here, we are using plain ascii text.
    * Applications can choose to use other formats like protobuf, JSON, XML, etc.
    */

esp_err_t _mqtt_config_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                    uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    if (inbuf) {
        //ESP_LOGI(TAG, "Received data: \"%.*s\"", inlen, (char *)inbuf);

        // store values in non volatile storage (flash)
        char * str = strndup((const char * const)inbuf, inlen);
        {
            if (strcmp(str, "null") == 0) {
                str = strdup("\0");
            } 
            ESP_LOGI(TAG, "%s Received MQTT_URL: (%s)", __FUNCTION__, str);
            nvs_handle_t nvs_handle;
            ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
            ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "mqtt_url", str));
            ESP_ERROR_CHECK(nvs_commit(nvs_handle));
            nvs_close(nvs_handle);
        }
        free(str);
    }

    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL) {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
}

esp_err_t _mqtt_status_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                               uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    ESP_LOGI(TAG, "%s called", __FUNCTION__);

    if (inbuf) {
        ESP_LOGI(TAG, "%s Received data: %.*s", __FUNCTION__, inlen, (char *)inbuf);
    }
    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL) {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; // +1 for NULL terminating byte

    return ESP_OK;
}

esp_err_t _ota_status_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                              uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    ESP_LOGI(TAG, "%s called", __FUNCTION__);

    if (inbuf) {
        ESP_LOGI(TAG, "%s Received data: %.*s", __FUNCTION__, inlen, (char *)inbuf);
    }
    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL) {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
}

esp_err_t
ble_prov_start_provisioning(const char *ble_device_name_prefix, int security, char const * const pop)
{
    // register event handlers for WiFi, IP and Provisioning related events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));

    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        //.scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM  // app doesn't require BT/BLE
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE  // test
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
#if 0    
    ESP_ERROR_CHECK(wifi_prov_mgr_disable_auto_stop(100));  //  the provisioning service will only be stopped after an explicit call to wifi_prov_mgr_stop_provisioning()
#endif

    bool provisioned;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");
        _prov_event_group = xEventGroupCreate();
    
        // find the Device Service Name that we want (the `device name`)
        char service_name[12];
        _get_device_service_name(service_name, sizeof(service_name));

        const char *service_key = NULL; // ignored

        /* Set a custom 128 bit UUID which will be included in the BLE advertisement and will correspond to the primary
         * GATT service that provides provisioning endpoints as GATT characteristics. Each GATT characteristic will be
         * formed using the primary service UUID as base, with different auto assigned 12th and 13th bytes (assume
         * counting starts from 0th byte). 
         * The client side applications must identify the endpoints by reading the User Characteristic Description
         * descriptor (0x2901) for each characteristic, which contains the endpoint name of the characteristic */
        uint8_t custom_service_uuid[] = {
            // LSB <------------------------------------------------------------------------------> MSB
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf, 0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };

        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

        // endpoints for additional custom data
        // see: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html
        wifi_prov_mgr_endpoint_create("mqtt-config");
        wifi_prov_mgr_endpoint_create("mqtt-status");
        wifi_prov_mgr_endpoint_create("ota-status");

        // start provisioning service
        // (de-init is trigged by the default event loop handler)
        ESP_LOGW(TAG, "Calling wifi_prov_mgr_start_provisioning() ..");
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key));

        wifi_prov_mgr_endpoint_register("mqtt-config", _mqtt_config_handler, NULL);
        wifi_prov_mgr_endpoint_register("mqtt-status", _mqtt_status_handler, NULL);
        wifi_prov_mgr_endpoint_register("ota-status", _ota_status_handler, NULL);

        // wait until provisioning is completed
        xEventGroupWaitBits(_prov_event_group, WIFI_PROV_END_EVENT, false, true, portMAX_DELAY);

        ESP_LOGW(TAG, "Provisioning done.");

    } else {

        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

        // we don't need the manager as device is already provisioned, so let's release it's resources
        wifi_prov_mgr_deinit();
    }

    return ESP_OK;
}
