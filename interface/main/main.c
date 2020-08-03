/**
 * @brief Main App: Pool Interface using ESP32
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <ota_update_task.h>
#include <wifi_connect.h>
#include <factory_reset_task.h>

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
_initNvsFlash(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static esp_err_t
_wifi_connect_cb(void * const priv_void, esp_ip4_addr_t const * const ip)
{
    wifi_connect_priv_t * const priv = priv_void;
    ipc_t * const ipc = priv->ipc;

    snprintf(ipc->dev.ipAddr, WIFI_DEVIPADDR_LEN, IPSTR, IP2STR(ip));
	board_name(ipc->dev.name, WIFI_DEVNAME_LEN);

    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
    httpd_config.uri_match_fn = httpd_uri_match_wildcard;
    ESP_ERROR_CHECK(httpd_start(&priv->httpd_handle, &httpd_config));

    httpd_register_cb(priv->httpd_handle, ip, ipc);

    ipc->dev.count.wifiConnect++;
    return ESP_OK;
}

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
        // should probably reprovision on repeated auth_err and return ESP_FAIL
    }
    ESP_LOGW(TAG, "Wifi disconnect connectCnt=%u, authErrCnt=%u", ipc->dev.count.wifiConnect, ipc->dev.count.wifiAuthErr);
    return ESP_OK;
}

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
    ESP_ERROR_CHECK(wifi_connect_start(NULL));
}

void
app_main()
{
    _initNvsFlash();

    ESP_LOGI(TAG, "starting ..");
    xTaskCreate(&factory_reset_task, "factory_reset_task", 4096, NULL, 5, NULL);

    static ipc_t ipc = {};
    ipc.to_pool_q = xQueueCreate(10, sizeof(ipc_to_pool_msg_t));
    ipc.to_mqtt_q = xQueueCreate(40, sizeof(ipc_to_mqtt_msg_t));
    assert(ipc.to_mqtt_q && ipc.to_pool_q);

    poolstate_init();

    _connect2wifi_and_start_httpd(&ipc);

    // from here the tasks take over

    xTaskCreate(&ota_update_task, "ota_update_task", 4096, NULL, 5, NULL);
    xTaskCreate(&mqtt_task, "mqtt_task", 2*4096, &ipc, 5, NULL);
    xTaskCreate(&pool_task, "pool_task", 2*4096, &ipc, 5, NULL);
    xTaskCreate(&hass_task, "hass_task", 2*4096, &ipc, 5, NULL);
}