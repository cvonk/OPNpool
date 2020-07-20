/**
 * @brief Pool Interface using ESP32
 *
 * Sniffs the Pentair RS-485 bus to collect pool state information.  The pool state is output
 * in JavaScript Object Notation (JSON).  The Arduino program allows sending select parameters
 * to the pool controller.
 *
 * Platform: ESP32 using Espressif ESP-IDF SDK
 * Documentation : https://coertvonk.com/sw/embedded/pentair-interface-11554
 *
 * Getting started
 *   Install Microsoft Visual Studio Code
 *     - add Microsoft C/C++ extension
 *     - add Espressif IDF" extension, see
 *       https://github.com/cvonk/vscode-starters/blob/master/ESP32/README.md
 *
 * VS Code
 *   Checkout a local copy of 'ESP32-SDK_Pool-interface"
 *     git clone ssh://nas/volume2/git/cvonk/ESP32-Arduino_Pool-interface.git
 *     (Do not use a copy on the file server.  It causes problems and is slow)
 *   Start Visual Studio Code (vscode), in directory
 *     ESP32-SDK_Pool-interface\interface
 *   Start compile/flash/monitor, using keyboard shortcut ctrl-e d
 *
 * VS Code JTAG Debugging
 *   Find a board with a JTAG connector, either
 *     - ESP-WROVER-KIT, the micro-USB port carries both JTAG and Serial
 *         - make sure the jumpers are set for JTAG
 *     - My Pool Interface board (build around Wemos LOLIN D32), the micro USB
 *       carries Serial and the 10-pin connector carries JTAG.  Connect it
 *       using a short (6 inch) cable to ESP-PROG that connects to your computer
 *       using miro-USB
 *         - see https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP-Prog_guide_en.md
 *         - set `JTAG PWR SEL` jumper for 3.3V
 *         - we will not use the PROG interface, so the jumpers are `don't care
 *   Connect Serial and JTAG to your computer
 *     Use Windows device manageer to find (and opt change) the name of the COM
 *     port and update `settings.json` accordingly.
 *   Install the FTDI D2xx driver and use Zadig as described in "JTAG Debugging"
 *     https://github.com/cvonk/vscode-starters/blob/master/ESP32/README.md
 *   Debugging the OTA image is a bit tricky, because you don't know the offset in flash.
 *   Instead we load this "interrace app" as a factory app.
 *     1. clear the OTA apps in flash, so that they don't start instead
 *         - open a ESP-IDF terminal ([F1] ESP-IDF: Open ESP-IDF Terminal)
 *         - idf.py erase_otadata
 *     2. remove https://coertvonk.com/cvonk/pool/interface.bin on the server
 *     3. provision the board
 *         - open "factory" folder in another vscode instance
 *         - build, flash and monitor (ctrl-e d) the `factory` app
 *         - use the Android app to provision the WiFi credentials
 *     4. run the interface app on the board
 *         - open "interface folder" in a vscode instance
 *         - build, flash and monitor (ctrl-e d) the `interface` app
 *         - if you haven't already, connect the JTAG interface
 *         - in the "debug" panel, click `Launch`
 *    Start debugging using JTAG
 *      - Built/upload/monitor
 *      - Debug > Launch
 *          - note that after hitting a break point, you may still have to
 *            select the corresponding task
 *
 * Board: Wemos LOLIN D32 (ESP32-WVROOM)
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2019, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <stdio.h>
#include <sdkconfig.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <ota_update_task.h>
#include <wifi_connect.h>
#include <esp_http_server.h>

#include <factory_reset_task.h>
#include "board_name.h"
#include "httpd_cb.h"
#include "mqtt_task.h"
#include "ipc_msgs.h"

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

#if 0
/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \

enum http_method
  {
#define XX(num, name, string) HTTP_##name = num,
  HTTP_METHOD_MAP(XX)
#undef XX
  };
static const char *method_strings[] =
  {
#define XX(num, name, string) #string,
  HTTP_METHOD_MAP(XX)
#undef XX
  };

#ifndef ELEM_AT
# define ELEM_AT(a, i, v) ((unsigned int) (i) < ARRAY_SIZE(a) ? (a)[(i)] : (v))
#endif

static const char *
http_method_str (enum http_method m)
{
  return ELEM_AT(method_strings, m, "<unknown>");
}
#endif

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

    static httpd_uri_t httpdGet = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = httpd_cb,
    };
    static httpd_uri_t httpdPost = {
        .uri       = "/*",
        .method    = HTTP_POST,
        .handler   = httpd_cb,
    };
    httpdGet.user_ctx = ipc;
    httpdPost.user_ctx = ipc;

    ESP_LOGI(TAG, "Listening at http://" IPSTR "/* for GET/POST", IP2STR(ip));
    ESP_ERROR_CHECK(httpd_register_uri_handler(priv->httpd_handle, &httpdGet));
    ESP_ERROR_CHECK(httpd_register_uri_handler(priv->httpd_handle, &httpdPost));

    ipc->dev.count.wifiConnect++;
    ESP_LOGI(TAG, "WiFi connect ipAddr=%s, devName=%s, connectCnt=%u", ipc->dev.ipAddr, ipc->dev.name, ipc->dev.count.wifiConnect);
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
    ipc.toMqttQ = xQueueCreate(2, sizeof(toMqttMsg_t));
    assert(ipc.toMqttQ);

    _connect2wifi_and_start_httpd(&ipc);

    // from here the tasks take over

    xTaskCreate(&ota_update_task, "ota_update_task", 4096, NULL, 5, NULL);
    xTaskCreate(&mqtt_task, "mqtt_task", 2*4096, &ipc, 5, NULL);
}