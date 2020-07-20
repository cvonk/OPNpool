/**
 * @brief Provides wifi_prov_config_handlers_t to protocomm
 *
 * based on: ...\espressif\esp-idf-v4.1-beta2\examples\provisioning\legacy\ble_prov\main\ble_prov_handlers.c
   SoftAP based Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* This file is mostly a boiler-plate code that applications can use without much change */

#include <stdio.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <wifi_provisioning/wifi_config.h>

#include "factory_ble_prov.h"

static char const * const TAG = "ble_prov_handler";

struct wifi_prov_ctx {
    wifi_config_t wifi_cfg;
};

static wifi_config_t *
_get_config(wifi_prov_ctx_t **ctx)
{
    return (*ctx ? &(*ctx)->wifi_cfg : NULL);
}

static wifi_config_t *
_new_config(wifi_prov_ctx_t **ctx)
{
    free(*ctx);
    (*ctx) = (wifi_prov_ctx_t *) calloc(1, sizeof(wifi_prov_ctx_t));
    return _get_config(ctx);
}

static void
_free_config(wifi_prov_ctx_t **ctx)
{
    free(*ctx);
    *ctx = NULL;
}

static esp_err_t
_get_status_handler(wifi_prov_config_get_data_t * resp_data, wifi_prov_ctx_t * * ctx)
{
    memset(resp_data, 0, sizeof(wifi_prov_config_get_data_t));

    if (ble_prov_get_wifi_state(&resp_data->wifi_state) != ESP_OK) {
        ESP_LOGW(TAG, "Prov app not running");
        return ESP_FAIL;
    }

    if (resp_data->wifi_state == WIFI_PROV_STA_CONNECTED) {
        ESP_LOGI(TAG, "Connected state");

        /* IP Addr assigned to STA */
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
        esp_ip4addr_ntoa(&ip_info.ip, resp_data->conn_info.ip_addr, sizeof(resp_data->conn_info.ip_addr));

        /* AP information to which STA is connected */
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        memcpy(resp_data->conn_info.bssid, (char *)ap_info.bssid, sizeof(ap_info.bssid));
        memcpy(resp_data->conn_info.ssid,  (char *)ap_info.ssid,  sizeof(ap_info.ssid));
        resp_data->conn_info.channel   = ap_info.primary;
        resp_data->conn_info.auth_mode = ap_info.authmode;
    } else if (resp_data->wifi_state == WIFI_PROV_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected state");
        ble_prov_get_wifi_disconnect_reason(&resp_data->fail_reason);
    } else {
        ESP_LOGI(TAG, "Connecting ..");  // leave to main_app to handle this
    }
    return ESP_OK;
}

static esp_err_t
_set_config_handler(const wifi_prov_config_set_data_t * req_data, wifi_prov_ctx_t * * ctx)
{
    wifi_config_t *wifi_cfg = _get_config(ctx);
    if (wifi_cfg) {
        _free_config(ctx);
    }

    wifi_cfg = _new_config(ctx);
    if (!wifi_cfg) {
        ESP_LOGE(TAG, "Unable to alloc wifi config");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Received WiFi credentials:\n\tssid %s \n\tpassword %s", req_data->ssid, req_data->password);

    strncpy((char *) wifi_cfg->sta.ssid, req_data->ssid, sizeof(wifi_cfg->sta.ssid));
    strlcpy((char *) wifi_cfg->sta.password, req_data->password, sizeof(wifi_cfg->sta.password));
    return ESP_OK;
}

static esp_err_t
_apply_config_handler(wifi_prov_ctx_t * * ctx)
{
    wifi_config_t * const wifi_cfg = _get_config(ctx);
    if (!wifi_cfg) {
        ESP_LOGE(TAG, "WiFi config not set");
        return ESP_FAIL;
    }
    ble_prov_configure_sta(wifi_cfg);
    ESP_LOGI(TAG, "WiFi Credentials Applied");

    _free_config(ctx);
    return ESP_OK;
}

wifi_prov_config_handlers_t protocomm_handlers = {
    .get_status_handler   = _get_status_handler,
    .set_config_handler   = _set_config_handler,
    .apply_config_handler = _apply_config_handler,
    .ctx = NULL
};
