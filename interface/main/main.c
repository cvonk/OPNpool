/**
 * @brief OPNpool, main app: Pool Interface using ESP32
 *
 * © Copyright 2014, 2019, 2022, Coert Vonk
 * 
 * This file is part of OPNpool.
 * 
 * OPNpool is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * 
 * OPNpool is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with OPNpool. 
 * If not, see <https://www.gnu.org/licenses/>.
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <sys/param.h>
//#include <esp_sysview_trace.h>
//#include <esp_heap_trace.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "ota_update_task.h"
#include "wifi_connect.h"
#include "factory_reset_task.h"

#include "datalink/datalink.h"
#include "network/network.h"
#include "poolstate/poolstate.h"
#include "httpd/httpd.h"
#include "utils/utils.h"
#include "ipc/ipc.h"

#include "pool_task.h"
#include "mqtt_task.h"
#include "hass_task.h"

static char const * const TAG = "main";

typedef struct wifi_connect_priv_t {
    ipc_t * ipc;
    httpd_handle_t httpd_handle;
} wifi_connect_priv_t;

static void
_init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

/*
 * Start a wildcard HTTP server, calling `httpd_register_handlers` to register the
 * URI handlers for incoming GET requests.
 */

static esp_err_t
_wifi_connect_cb(void * const priv_void, esp_ip4_addr_t const * const ip)
{
    wifi_connect_priv_t * const priv = priv_void;
    ipc_t * const ipc = priv->ipc;

    // note the MAC and IP addresses

    snprintf(ipc->dev.ipAddr, WIFI_DEVIPADDR_LEN, IPSTR, IP2STR(ip));
	board_name(ipc->dev.name, WIFI_DEVNAME_LEN);

    // start wildchard HTTP server

    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
    httpd_config.uri_match_fn = httpd_uri_match_wildcard;
    ESP_ERROR_CHECK(httpd_start(&priv->httpd_handle, &httpd_config));

    httpd_register_handlers(priv->httpd_handle, ip, ipc);

    ipc->dev.count.wifiConnect++;
    return ESP_OK;
}

/*
 * Stop the wildcard HTTP server
 */

static esp_err_t
_wifi_disconnect_cb(void * const priv_void, bool const auth_err)
{
    wifi_connect_priv_t * const priv = priv_void;
    ipc_t * const ipc = priv->ipc;

    if (priv->httpd_handle) {
        httpd_stop(priv->httpd_handle);
        priv->httpd_handle = NULL;
    }
    if (auth_err) {
        ipc->dev.count.wifiAuthErr++;
        // 2BD: should probably reprovision on repeated auth_err and return ESP_FAIL
    }
    ESP_LOGW(TAG, "Wifi disconnect connectCnt=%u, authErrCnt=%u", ipc->dev.count.wifiConnect, ipc->dev.count.wifiAuthErr);
    return ESP_OK;
}

static void
__attribute__((noreturn)) _delete_task()
{
    ESP_LOGI(TAG, "Exiting task ..");
    (void)vTaskDelete(NULL);

    while (1) {  // FreeRTOS requires that tasks never return
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/*
 * Connect to WiFi accesspoint.
 * Register callbacks when connected or disconnected.
 */

static void
_connect2wifi_and_start_httpd(ipc_t * const ipc)
{
    static wifi_connect_priv_t priv = {};
    priv.ipc = ipc;

    wifi_connect_config_t wifi_connect_config = {
        .onConnect = _wifi_connect_cb,
        .onDisconnect = _wifi_disconnect_cb,
        .priv = &priv,
    };
    ESP_ERROR_CHECK(wifi_connect_init(&wifi_connect_config));

    wifi_config_t * wifi_config_addr = NULL;
#ifdef CONFIG_OPNPOOL_HARDCODED_WIFI_CREDENTIALS
    if (strlen(CONFIG_OPNPOOL_HARDCODED_WIFI_SSID)) {
        ESP_LOGW(TAG, "Using SSID from Kconfig");
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = CONFIG_OPNPOOL_HARDCODED_WIFI_SSID,
                .password = CONFIG_OPNPOOL_HARDCODED_WIFI_PASSWD,
            }
        };
        wifi_config_addr = &wifi_config;
    } else
#endif
    {
        ESP_LOGW(TAG, "Using SSID from nvram");
    }

    esp_err_t err = wifi_connect_start(wifi_config_addr);
    if (err == ESP_ERR_WIFI_SSID) {
        ESP_LOGE(TAG, "Wi-Fi SSID/passwd not provisioned");
        _delete_task();
    }
    ESP_ERROR_CHECK(err);
}

#if 0
#define NUM_RECORDS 100
static heap_trace_record_t trace_record[NUM_RECORDS]; // This buffer must be in internal RAM
#endif

void
app_main()
{
    _init_nvs();

    ESP_LOGI(TAG, "starting ..");
    xTaskCreate(&factory_reset_task, "factory_reset_task", 4096, NULL, 5, NULL);

    static ipc_t ipc = {};
    ipc.to_pool_q = xQueueCreate(10, sizeof(ipc_to_pool_msg_t));
    ipc.to_mqtt_q = xQueueCreate(60, sizeof(ipc_to_mqtt_msg_t));
    assert(ipc.to_mqtt_q && ipc.to_pool_q);

    poolstate_init();

    _connect2wifi_and_start_httpd(&ipc);

#if 0
    // redirect log messages to the host using SystemView tracing module
    //esp_log_set_vprintf(&esp_sysview_vprintf);
    // init host-based heap tracing
    if (heap_trace_init_standalone(trace_record, NUM_RECORDS) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init heap trace!");
        return;
    }
    //ESP_LOGW(TAG, "step 1");
    //heap_trace_start(HEAP_TRACE_ALL);  // stopped using MQTT `htstop` command
    //ESP_LOGW(TAG, "step 2");
#endif

    // from here the tasks take over

    xTaskCreate(&ota_update_task, "ota_update_task", 4096, "interface", 5, NULL);
    xTaskCreate(&mqtt_task, "mqtt_task", 2*4096, &ipc, 5, NULL);
    xTaskCreate(&pool_task, "pool_task", 2*4096, &ipc, 5, NULL);
    xTaskCreate(&hass_task, "hass_task", 2*4096, &ipc, 5, NULL);
}