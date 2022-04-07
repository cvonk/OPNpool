/**
 * @brief Provision WiFi using BLE and phone app
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

#include <string.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <factory_ble_prov.h>

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

static void
_handle_wifi_sta_start(void * arg, esp_event_base_t event_base,
                       int event_id, void * event_data)
{
    esp_wifi_connect();
}

static void
_handle_wifi_sta_disconnect(void * arg, esp_event_base_t event_base,
                            int event_id, void * event_data)
{
    wifi_event_sta_disconnected_t * disconnected = (wifi_event_sta_disconnected_t *) event_data;

    switch (disconnected->reason) {
        case WIFI_REASON_NO_AP_FOUND:
            ESP_LOGW(TAG, "WiFi AP not found");
            if (_retry_num.ap_not_found < BLE_PROV_RECONN_ATTEMPTS) {
                _retry_num.ap_not_found++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "retry connect to WiFi ..");
            }
            break;
        case WIFI_REASON_AUTH_EXPIRE:
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_BEACON_TIMEOUT:
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_ASSOC_FAIL:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            ESP_LOGW(TAG, "WiFi auth error");
            if (_retry_num.ap_auth_fail < BLE_PROV_RECONN_ATTEMPTS) {
                _retry_num.ap_auth_fail++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "retry connect to WiFi ..");
            } else {
                _start_provisioning();  // Restart provisioning if authentication fails
            }
            break;
        default:
            esp_wifi_connect();
            break;
    }
}

static void
_handle_sta_got_ip(void * arg, esp_event_base_t event_base,
                   int event_id, void * event_data)
{
    ip_event_got_ip_t const * const event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(TAG, "IP addr " IPSTR, IP2STR(&event->ip_info.ip));
    _retry_num.ap_not_found = _retry_num.ap_auth_fail = 0;

    // this would be a great place to start OTA Update
    //xTaskCreate(&ota_update_task, "ota_update_task", 8192, NULL, 5, NULL);
}

static void
_init_wifi()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();  // init WiFi with configuration from non-volatile storage
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &_handle_wifi_sta_start, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_handle_wifi_sta_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_handle_sta_got_ip, NULL));
}

static void
_wifi_init_sta(void)
{
    ESP_LOGI(TAG, "starting WiFi");

    // start Wi-Fi in station mode with credentials set during provisioning
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main(void)
{
    _init_nvs();
    _init_wifi();

    bool provisioned;
    if (ble_prov_is_provisioned(&provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "Can't get device prov state");
        return;
    }
    if (provisioned) {
        _wifi_init_sta();
    } else {
        _start_provisioning();
    }
}
