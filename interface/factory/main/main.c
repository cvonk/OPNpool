/* based on: ...\espressif\esp-idf-v4.1-beta2\examples\provisioning\legacy\ble_prov\main\ble_main.c
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
#include <lwip/err.h>
#include <lwip/sys.h>

#include "ble_prov.h"
#include "ota_task.h"

/**
 * ble_device_name_prefix and BLE_PROV_POP must match build.gradle in the android app;
 * SECURITY_VERSION must correspond to View > Tool Windows > Build Variants > app = bleSec?Debug in
 * Android Studio.
 */
#define BLE_PROV_SECURITY_VERSION (1)
#define BLE_PROV_POP ("bqeL7mTz")
#define BLE_PROV_RECONN_ATTEMPTS (3)
static char const * ble_device_name_prefix = "POOL_";  // so we can use the Pool Configurator Android app, generic is "PROV_"

static char const * const TAG = "factory";

static void
_start_ble_provisioning()
{
    ESP_LOGI(TAG, "Starting BLE provisioning");
    static protocomm_security_pop_t const pop = {
        .data = (uint8_t const * const) BLE_PROV_POP,
        .len = (sizeof(BLE_PROV_POP)-1)
    };
    ESP_ERROR_CHECK( ble_prov_start_ble_provisioning(ble_device_name_prefix, BLE_PROV_SECURITY_VERSION, &pop) );
}

static void _event_handler(void* arg, esp_event_base_t event_base,
                          int event_id, void* event_data)
{
    static int s_retry_num_ap_not_found = 0;
    static int s_retry_num_ap_auth_fail = 0;
    //ble_prov_event_handler(arg, event_base, event_id, event_data);   // invoke provisioning event handler first

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
        switch (disconnected->reason) {
        case WIFI_REASON_AUTH_EXPIRE:
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_BEACON_TIMEOUT:
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_ASSOC_FAIL:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            ESP_LOGW(TAG, "connect to the AP fail : auth Error");
            if (s_retry_num_ap_auth_fail < BLE_PROV_RECONN_ATTEMPTS) {
                s_retry_num_ap_auth_fail++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "retry connecting to the AP...");
            } else {
                /* Restart provisioning if authentication fails */
                _start_ble_provisioning();
            }
            break;
        case WIFI_REASON_NO_AP_FOUND:
            ESP_LOGW(TAG, "connect to the AP fail : not found");
            if (s_retry_num_ap_not_found < BLE_PROV_RECONN_ATTEMPTS) {
                s_retry_num_ap_not_found++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "retry to connecting to the AP...");
            }
            break;
        default:
            /* None of the expected reasons */
            esp_wifi_connect();
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num_ap_not_found = 0;
        s_retry_num_ap_auth_fail = 0;
        xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
    }
}

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
_init_wifi()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();  // init WiFi with configuration from non-volatile storage
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_event_handler, NULL));



}

static void
_wifi_init_sta(void)
{
    /* Start Wi-Fi in station mode with credentials set during provisioning */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main(void)
{
    _init_nvs();
    _init_wifi();

    /* Check if device is provisioned */
    bool provisioned;
    if (ble_prov_is_provisioned(&provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "Error getting device provisioning state");
        return;
    }

    if (provisioned == false) {
        ESP_LOGI(TAG, "Starting BLE provisioning");
        _start_ble_provisioning();
    } else {
        ESP_LOGI(TAG, "Starting WiFi station");
        _wifi_init_sta();
    }
}
