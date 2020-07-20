/**
  * @brief demonstrates the use of the "wifi_connect" component
  **/
// Copyright © 2020, Coert Vonk
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <esp_netif.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_http_server.h>

#include <wifi_connect.h>

static char const * const TAG = "main";

typedef struct wifi_connect_priv_t {
    uint connectCnt;
    httpd_handle_t httpd_handle;
    httpd_uri_t * httpd_uri;
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
_httpd_handler(httpd_req_t *req)
{
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }
    const char * resp_str = "<html>\n"
        "<head>\n"
        "</head>\n"
        "<body>\n"
        "  <h1>Welcome to device world</h1>\n"
        "</body>\n"
        "</html>";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static void
_wifi_connect_on_connect(void * const priv_void, esp_ip4_addr_t const * const ip)
{
    wifi_connect_priv_t * const priv = priv_void;
    priv->connectCnt++;
    ESP_LOGI(TAG, "WiFi connectCnt = %u", priv->connectCnt);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    esp_err_t err = httpd_start(&priv->httpd_handle, &config);
    if (err != ESP_OK) {
       ESP_LOGI(TAG, "Error starting server!");
       return;
    }
    httpd_register_uri_handler(priv->httpd_handle, priv->httpd_uri);
    ESP_LOGI(TAG, "Listening at http://" IPSTR "/", IP2STR(ip));
}

static void
_wifi_connect_on_disconnect(void * const priv_void)
{
    wifi_connect_priv_t * const priv = priv_void;
    httpd_stop(priv->httpd_handle);
}

void
app_main(void)
{
    _initNvsFlash();

    httpd_uri_t httpd_uri = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = _httpd_handler,
        .user_ctx  = NULL
    };
    wifi_connect_priv_t priv = {
        .connectCnt = 0,
        .httpd_uri = &httpd_uri,
    };
    wifi_connect_config_t wifi_connect_config = {
        .onConnect = _wifi_connect_on_connect,
        .onDisconnect = _wifi_connect_on_disconnect,
        .priv = &priv,
    };
    ESP_ERROR_CHECK(wifi_connect(&wifi_connect_config));

    ESP_LOGI(TAG, "Connected");
    while (1) {
        // do something
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}