/* BLE based provisioning and OTA updates

   API: ESP-IDF v3.3
  
   make partition_table  # show partition table layout
   make erase_flash      # erase previous OTA partitions, of just "make erase_ota" to erase ota_data partition

   more info at https://docs.espressif.com/projects/esp-jumpstart/en/latest/networkconfig.html
   BLE based Provisioning based on PD/CC0 licensed https://github.com/espressif/esp-idf/blob/v3.2/examples/provisioning/ble_prov/
 */

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <lwip/err.h>
#include <lwip/sys.h>
#include <esp_ota_ops.h>

#include "ble_prov.h"
#include "pool_ota.h"

static const char *TAG = "pool_factory";

/**
 * ble_device_name_prefix and proof_of_possesion must match build.gradle in the android app;
 * SECURITY_VERSION must correspond to View > Tool Windows > Build Variants > app = bleSec?Debug in
 * Android Studio.
 */
#define SECURITY_VERSION (1)
#define proof_of_possesion "bqeL7mTz"
#define AP_RECONN_ATTEMPTS (3)
static char const * ble_device_name_prefix = "POOL_";

#if 0
static void _wifi_init_sta()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );  // start WiFi in station mode with credentials set during provisioning
    ESP_ERROR_CHECK(esp_wifi_start() );
}
#endif

/**
 * @brief Start provisioning over BLE using matching Android app
 * 
 */
static void _start_ble_provisioning()
{
    static protocomm_security_pop_t const pop = {
        .data = (uint8_t const * const) proof_of_possesion,
        .len = (sizeof(proof_of_possesion)-1)
    };

    ESP_ERROR_CHECK( ble_prov_start_ble_provisioning(ble_device_name_prefix, SECURITY_VERSION, &pop) );
}


static esp_err_t _event_handler(void *ctx, system_event_t *event)
{
    static int s_retry_num_ap_auth_fail = 0;
    ble_prov_event_handler(ctx, event);   // invoke provisioning event handler first

    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            s_retry_num_ap_auth_fail = 0;
            xTaskCreate(&pool_ota_task, "pool_ota_task", 8192, NULL, 5, NULL);
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d", MAC2STR(event->event_info.sta_connected.mac), event->event_info.sta_connected.aid);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR" leave, AID=%d", MAC2STR(event->event_info.sta_disconnected.mac), event->event_info.sta_disconnected.aid);
             if (s_retry_num_ap_auth_fail < AP_RECONN_ATTEMPTS) {
                s_retry_num_ap_auth_fail++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "retry connecting to the AP...");
            } else {                
                _start_ble_provisioning();  // restart provisioning if authentication fails
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

/**
 * @brief Initialize non-volatile storage
 * 
 * We use a non-default partition table with a smaller NVS partition.  This may cause NVS initialization
 * to fail.  Also the format of the data might have changed between API versions.  If any of these happen
 * we erase NVS partition and initialize NVS again.
 */
static void _init_nvs() 
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
}

/**
 * @brief Init WiFi with configuration from non-volatile storage
 */
static void _init_wifi()
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(_event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
}


/**
 * @brief Provision WiFi SSID/passwd using BLE and start app using OTA download
 * 
 * Provisioning is done using an acompanying Android app.  Once the WiFi is up and running,
 * the SYSTEM_EVENT_STA_GOT_IP event will trigger and OTA download of the app.
 */
void app_main()
{
    _init_nvs();
    _init_wifi();

    _start_ble_provisioning();    
}

