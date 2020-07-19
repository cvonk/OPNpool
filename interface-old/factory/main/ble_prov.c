/**
 * @brief Interface with Android/iOS app to provision WiFi
 *
   based on: ...\espressif\esp-idf-v4.1-beta2\examples\provisioning\legacy\ble_prov\main\ble_prov.c
   BLE based Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_bt.h>
#include <esp_event.h>
#include <protocomm.h>
#include <protocomm_ble.h>
#include <protocomm_security0.h>
#include <protocomm_security1.h>
#include <wifi_provisioning/wifi_config.h>

#include "ble_prov.h"
#include "protocomm_handlers.h"

static char const * const TAG = "ble_prov";

struct ble_prov_data {
    protocomm_t * pc;                      /*!< Protocomm handler */
    int security;                          /*!< Type of security to use with protocomm */
    const protocomm_security_pop_t * pop;  /*!< Pointer to proof of possession */
    esp_timer_handle_t timer;              /*!< Handle to timer */
    wifi_prov_sta_state_t wifi_state;
    wifi_prov_sta_fail_reason_t wifi_disconnect_reason;
};

/* Pointer to provisioning application data */
static struct ble_prov_data * _provisioning;

static esp_err_t
_ble_prov_start_service(const char * ble_device_name_prefix)
{
    _provisioning->pc = protocomm_new();
    if (_provisioning->pc == NULL) {
        ESP_LOGE(TAG, "Failed to create new protocomm instance");
        return ESP_FAIL;
    }
    protocomm_ble_name_uuid_t nu_lookup_table[] = {
        {"prov-session", 0xFF51},
        {"prov-config",  0xFF52},
        {"proto-ver",    0xFF53},
    };

#if 1 /* the old uuid */
    protocomm_ble_config_t config = {
        .service_uuid = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
            0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        },
        .nu_lookup_count = sizeof(nu_lookup_table)/sizeof(nu_lookup_table[0]),
        .nu_lookup = nu_lookup_table
    };
#else
    /* Config for protocomm_ble_start() */
    protocomm_ble_config_t config = {
        .service_uuid = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        },
        .nu_lookup_count = sizeof(nu_lookup_table)/sizeof(nu_lookup_table[0]),
        .nu_lookup = nu_lookup_table
    };

    /* With the above selection of 128bit primary service UUID and
     * 16bit endpoint UUIDs, the 128bit characteristic UUIDs will be
     * formed by replacing the 12th and 13th bytes of the service UUID
     * with the 16bit endpoint UUID, i.e. :
     *    service UUID : 021a9004-0382-4aea-bff4-6b3f1c5adfb4
     *     masked base : 021a____-0382-4aea-bff4-6b3f1c5adfb4
     * ------------------------------------------------------
     * resulting characteristic UUIDs :
     * 1) prov-session : 021a0001-0382-4aea-bff4-6b3f1c5adfb4
     * 2)  prov-config : 021a0002-0382-4aea-bff4-6b3f1c5adfb4
     * 3)    proto-ver : 021a0003-0382-4aea-bff4-6b3f1c5adfb4
     *
     * Also, note that each endpoint (characteristic) will have
     * an associated "Characteristic User Description" descriptor
     * with 16bit UUID 0x2901, each containing the corresponding
     * endpoint name. These descriptors can be used by a client
     * side application to figure out which characteristic is
     * mapped to which endpoint, without having to hardcode the
     * UUIDs */
#endif
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(config.device_name, sizeof(config.device_name), "%s%02X%02X%02X",
             ble_device_name_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);

    // release BT memory, as we need only BLE
    esp_err_t err = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (err) {
        ESP_LOGE(TAG, "bt_controller_mem_release failed %d", err);
        if (err != ESP_ERR_INVALID_STATE) {
            return err;
        }
    }

    // start protocomm layer on top of BLE
    if (protocomm_ble_start(_provisioning->pc, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start BLE provisioning");
        return ESP_FAIL;
    }

    protocomm_set_version(_provisioning->pc, "proto-ver", "V0.1");

    if (_provisioning->security == 0) {
        protocomm_set_security(_provisioning->pc, "prov-session", &protocomm_security0, NULL);
    } else if (_provisioning->security == 1) {
        protocomm_set_security(_provisioning->pc, "prov-session", &protocomm_security1, _provisioning->pop);
    }

    // when WiFi ssid/passwd set, ble_prov_configure_sta() is called (by protocomm_handlers.apply_config_handler)
    if (protocomm_add_endpoint(_provisioning->pc, "prov-config",
                               wifi_prov_config_data_handler,
                               (void *) &protocomm_handlers) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set provisioning endpoint");
        protocomm_ble_stop(_provisioning->pc);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "advertising as \"%s\"", config.device_name);
    return ESP_OK;
}

static void
_handle_wifi_sta_start(void * arg, esp_event_base_t event_base,
                       int event_id, void * event_data)
{
    if (!_provisioning) return;

    ESP_LOGI(TAG, "STA Start");
    /* Once configuration is received through protocomm,
        * device is started as station. Once station starts,
        * wait for connection to establish with configured
        * host SSID and password */
    _provisioning->wifi_state = WIFI_PROV_STA_CONNECTING;

}

static void
_handle_wifi_sta_disconnect(void * arg, esp_event_base_t event_base,
                            int event_id, void * event_data)
{
    if (!_provisioning) return;

    _provisioning->wifi_state = WIFI_PROV_STA_DISCONNECTED;

    wifi_event_sta_disconnected_t * disconnected = (wifi_event_sta_disconnected_t *) event_data;
    ESP_LOGE(TAG, "STA Disconnected (%d)", disconnected->reason);

    /* Set code corresponding to the reason for disconnection */
    switch (disconnected->reason) {
        case WIFI_REASON_AUTH_EXPIRE:
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_BEACON_TIMEOUT:
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_ASSOC_FAIL:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            ESP_LOGI(TAG, "STA Auth Error");
            _provisioning->wifi_disconnect_reason = WIFI_PROV_STA_AUTH_ERROR;
            break;
        case WIFI_REASON_NO_AP_FOUND:
            ESP_LOGI(TAG, "STA AP Not found");
            _provisioning->wifi_disconnect_reason = WIFI_PROV_STA_AP_NOT_FOUND;
            break;
        default:
            /* If none of the expected reasons, retry connecting to host SSID */
            _provisioning->wifi_state = WIFI_PROV_STA_CONNECTING;
            esp_wifi_connect();
    }
}

static void
_handle_sta_got_ip(void * arg, esp_event_base_t event_base,
                   int event_id, void * event_data)
{
    if (!_provisioning) return;

    ESP_LOGI(TAG, "STA Got IP");  // that means configuration was successful

    // Schedule timer to stop provisioning app after 30 seconds
    _provisioning->wifi_state = WIFI_PROV_STA_CONNECTED;
    if (_provisioning && _provisioning->timer) {
        esp_timer_start_once(_provisioning->timer, 30000U * 1000U);
    }
}

static void ble_prov_stop_service(void)
{
    protocomm_remove_endpoint(_provisioning->pc, "prov-config");
    protocomm_unset_security(_provisioning->pc, "prov-session");
    protocomm_unset_version(_provisioning->pc, "proto-ver");
    protocomm_ble_stop(_provisioning->pc);
    protocomm_delete(_provisioning->pc);

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_START, &_handle_wifi_sta_start));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_handle_wifi_sta_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &_handle_sta_got_ip));

    esp_bt_mem_release(ESP_BT_MODE_BTDM);  // release memory used by BT stack
}

static void stop_prov_task(void * arg)
{
    ESP_LOGI(TAG, "Stopping provisioning");
    ble_prov_stop_service();

    /* Timer not needed anymore */
    esp_timer_handle_t timer = _provisioning->timer;
    esp_timer_delete(timer);
    _provisioning->timer = NULL;

    /* Free provisioning process data */
    free(_provisioning);
    _provisioning = NULL;
    ESP_LOGI(TAG, "Provisioning stopped");

    vTaskDelete(NULL);
}

/* Callback to be invoked by timer */
static void _stop_prov_cb(void * arg)
{
    xTaskCreate(&stop_prov_task, "stop_prov", 2048, NULL, tskIDLE_PRIORITY, NULL);
}

esp_err_t
ble_prov_get_wifi_state(wifi_prov_sta_state_t* state)
{
    if (_provisioning == NULL || state == NULL) {
        return ESP_FAIL;
    }

    *state = _provisioning->wifi_state;
    return ESP_OK;
}

esp_err_t
ble_prov_get_wifi_disconnect_reason(wifi_prov_sta_fail_reason_t* reason)
{
    if (_provisioning == NULL || reason == NULL) {
        return ESP_FAIL;
    }

    if (_provisioning->wifi_state != WIFI_PROV_STA_DISCONNECTED) {
        return ESP_FAIL;
    }

    *reason = _provisioning->wifi_disconnect_reason;
    return ESP_OK;
}

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
        ESP_LOGI(TAG, "Found ssid %s",     (const char*) wifi_cfg.sta.ssid);
        ESP_LOGI(TAG, "Found password %s", (const char*) wifi_cfg.sta.password);
    }
    return ESP_OK;
}

esp_err_t
ble_prov_configure_sta(wifi_config_t * const wifi_cfg)
{
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode");
        return ESP_FAIL;
    }

    // configure WiFi station with host credentials provided during provisioning
    if (esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi configuration");
        return ESP_FAIL;
    }
    if (esp_wifi_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi configuration");
        return ESP_FAIL;
    }
    if (esp_wifi_connect() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect WiFi");
        return ESP_FAIL;
    }

    if (_provisioning) {
        _provisioning->wifi_state = WIFI_PROV_STA_CONNECTING;
    }
    return ESP_OK;
}

esp_err_t
ble_prov_start_provisioning(const char *ble_device_name_prefix, int security, const protocomm_security_pop_t *pop)
{
    if (_provisioning) {  // app is already running
        ESP_LOGI(TAG, "Invalid provisioning state");
        return ESP_FAIL;
    }

    _provisioning = (struct ble_prov_data *) calloc(1, sizeof(struct ble_prov_data));
    if (!_provisioning) {
        ESP_LOGI(TAG, "Unable to allocate prov data");
        return ESP_ERR_NO_MEM;
    }

    /* Initialize app data */
    _provisioning->pop = pop;
    _provisioning->security = security;

    /* Create timer object as a member of app data */
    esp_timer_create_args_t timer_conf = {
        .callback = _stop_prov_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "stop_ble_tm"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_conf, &_provisioning->timer));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &_handle_wifi_sta_start, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_handle_wifi_sta_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_handle_sta_got_ip, NULL));

    ESP_ERROR_CHECK(_ble_prov_start_service(ble_device_name_prefix));
    return ESP_OK;
}
