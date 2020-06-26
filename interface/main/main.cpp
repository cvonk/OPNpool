/**
 * @brief Pool Interface using ESP32
 *
 * Sniffs the Pentair RS-485 bus to collect pool state information.  The pool state is output
 * in JavaScript Object Notation (JSON).  The Arduino program allows sending select parameters
 * to the pool controller.
 *
 * Platform: ESP8266 using Arduino IDE
 * Documentation : https://coertvonk.com/sw/embedded/pentair-interface-11554
 * Tested with: Arduino IDE 1.6.11, board package esp8266 2.3.0, Adafruit huzzah esp8266
 *              Pentair SunTouch running firmware 2.80, connected intelliFlow pump and intelliChlor
 *
 * Your development environment:(GNU toolchain, ESP-IDF, JTAG/OpenOCD, VSCode):
 *   see https://github.com/cvonk/vscode-starters/tree/master/ESP32
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
 * Visual Studio Code:
 * - ESP32 debug see launch.json
 * - ext https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools
 * - ext https://marketplace.visualstudio.com/items?itemName=webfreak.debug
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2019, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <sdkconfig.h>

#include <stdio.h>
#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include <esp_ota_ops.h>
#include <esp_log.h>
#include <esp_http_server.h>

#include "cbuffer.h"      // circular buffer class CBUF
#include "pooltypes.h"
#include "receiver.h"
#include "poolstate.h"
#include "transmitqueue.h"

#include "ota_task.h"
#include "reset_task.h"
#include "pool_mdns.h"

static char const * const TAG = "pool_main";
static EventGroupHandle_t _wifi_event_group;
typedef enum {
    WIFI_EVENT_CONNECTED = BIT0
} wifi_event_types_t;

PoolState poolState; // shared between
TransmitQueue transmitQueue(poolState);  // so transmitQueue can access current state

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
_http_resp_root(httpd_req_t *req)
{
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    //const char * resp_str = (const char *) req->user_ctx;  // send string passed in user context as response
    const char * resp_str = "<html>\n"
        "<head>\n"
        "  <meta name='viewport' content='width=device-width, initial-scale=1, minimum-scale=1.0 maximum-scale=1.0' />\n"
        "  <script type='text/javascript'>\n"
        "    window.addEventListener('message', event => {\n"
        "      var iframe = document.getElementsByTagName('iframe')[0].contentWindow;\n"
        "      iframe.postMessage('toIframe', event.origin);\n"
        "    });\n"
        "  </script>\n"
        "</head>\n"
        "<body>\n"
        "  <style>\n"
        "    body {width:100%;height:100%;margin:0;overflow:hidden;background-color:#252525;}\n"
        "    #iframe {position:absolute;left:0px;top:0px;}\n"
        "  </style>\n"
        "  <h1>Page loading</h1>\n"
        "  <iframe id='iframe' name='iframe1' frameborder='0' width='100%' height='100%' src='//coertvonk.com/jvonk/pool'></iframe>\n"
        "</body>\n"
        "</html>";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static char *
_jsonProcessQueryVars(httpd_req_t * req, char * const buf)
{
    char * callback = NULL;  // uses JSON-P to get around CORS
    char * rest = NULL;
    for (char * key = strtok_r(buf, "&", &rest); key != NULL; key = strtok_r(NULL, "&", &rest)) {
        char * value = strchr(key, '=');
        if (value) {
            *value++ = '\0';
            if (strcmp(key, "callback") == 0) {
                callback = (char *) malloc(strlen(value)+1);
                if (callback) {
                    strcpy(callback, value);
                }
            }
            ESP_LOGI(TAG, "query var, %s=%s", key, value);
        }
#if 1
        transmitQueue.enqueue(key, value);  // MAKE SURE that the queue copies the values, as they will be freed soon
#endif
    }
    return callback;
}

// https://github.com/freebsd/freebsd/blob/master/sys/libkern/strlcpy.c
static size_t
_strlcpy(char * __restrict dst, const char * __restrict src, size_t dsize)
{
	const char *osrc = src;
	size_t nleft = dsize;

	if (nleft != 0) {  // Copy as many bytes as will fit
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}
	if (nleft == 0) { // not enough room in dst, add NUL and traverse rest of src
		if (dsize != 0)
			*dst = '\0';
		while (*src++)
			;
	}
	return(src - osrc - 1);	 // count does not include NUL
}

// https://github.com/freebsd/freebsd/blob/master/sys/libkern/strlcat.c
static size_t
_strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	while (n-- != 0 && *d != '\0')  // Find the end of dst and adjust bytes left but don't go past end
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';
	return(dlen + (s - src));  // count does not include NUL
}

static esp_err_t
_http_resp_json(httpd_req_t * req)
{
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    // process query vars

    char * callback = NULL;  // uses JSON-P to get around CORS
    size_t const buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char * buf = (char *) malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            callback = _jsonProcessQueryVars(req, buf);
        }
        free(buf);
    }

    // return pool state

    char resp[450];
    resp[0] = '\0';
    if (callback) _strlcpy(resp, callback, sizeof(resp));
    if (callback) _strlcat(resp, "(", sizeof(resp));
    uint_least8_t const len = strlen(resp);

    PoolState * const state = (PoolState *) req->user_ctx;
    state->getStateAsJson(resp + len, sizeof(resp) - len);

    if (callback) _strlcat(resp, ")", sizeof(resp));
    free(callback);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}


typedef struct {
    char * uri;
    char * key;
} api_variable_t;

static api_variable_t _api_variables[] = {
    {(char *)"/api/circuit/pool", (char *)"pool-active"},
    {(char *)"/api/circuit/aux1", (char *)"aux1-active"},
    {(char *)"/api/pool/sp", (char *)"pool-sp"},
    {(char *)"/api/pool/src", (char *)"pool-src"},
    {(char *)"/api/spa/sp", (char *)"spa-sp"},
    {(char *)"/api/spa/src", (char *)"spa-src"},
};
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static esp_err_t
_http_resp_api(httpd_req_t * req)
{
    if (req->method != HTTP_POST && req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    api_variable_t * variable = _api_variables;
    for (int ii = 0; ii < ARRAY_SIZE(_api_variables); ii++, variable++) {
        if (strcmp(req->uri, variable->uri) == 0 ) {

            if (req->method == HTTP_POST) {
                char value[16];  // body of POST message holds the new value
                size_t recv_size = MIN(req->content_len, sizeof(value));
                int ret = httpd_req_recv(req, value, recv_size);
                if (ret <= 0) {
                    if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                        httpd_resp_send_408(req);  // KISS don't retry, just give up
                    }
                    return ESP_FAIL;
                }
                value[recv_size] = '\0';
                ESP_LOGI(TAG, "API POST var: %s=%s", variable->key, value);
                transmitQueue.enqueue(variable->key, value);  // MAKE SURE that the queue copies the values, as they will be freed soon
            }
            ESP_LOGI(TAG, "API GET var: %s", variable->key);
            PoolState * const state = (PoolState *) req->user_ctx;

            char reply[40];
            if( strcmp(req->uri, "/api/circuit/pool") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": %s }",  state->getCircuitState("pool") ? "true" : "false");
            }
            else if( strcmp(req->uri, "/api/circuit/aux1") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": %s }",  state->getCircuitState("aux1") ? "true" : "false");
            }
            else if( strcmp(req->uri, "/api/pool/sp") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": %d }",  state->getSetPoint("pool"));
            }
            else if( strcmp(req->uri, "/api/pool/src") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": \"%s\" }",  state->getHeatSrc("pool"));
            }
            else if( strcmp(req->uri, "/api/spa/sp") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": %d }",  state->getSetPoint("spa"));
            }
            else if( strcmp(req->uri, "/api/spa/src") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": \"%s\" }",  state->getHeatSrc("spa"));
            }
            else {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No such API");
                return ESP_FAIL;
            }
            ESP_LOGI(TAG, "API reply %s(%d) %d", reply, strlen(reply), sizeof(reply));
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, reply, strlen(reply));
            return ESP_OK;
        }
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No such API");
    return ESP_FAIL;
}

typedef struct {
    char * uri;
    esp_err_t (*handler)(httpd_req_t * req);
} uri_dispatch_t;

static uri_dispatch_t _uri_dispatch[] = {
    { (char *)"/",  _http_resp_root },
    { (char *)"/json",  _http_resp_json },
    { (char *)"/api/*",  _http_resp_api }
};

// home assistant set the state via POST (and gets the state via GET)
static esp_err_t
_httpdHandler(httpd_req_t * req) {

    if (req->method != HTTP_POST && req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    uri_dispatch_t * dispatch = _uri_dispatch;
    for (int ii = 0; ii < ARRAY_SIZE(_uri_dispatch); ii++, dispatch++) {
        int const len = strlen(dispatch->uri);
        if (dispatch->uri[len-1] == '*' && strncmp(req->uri, dispatch->uri, len-1) == 0 ) {
            return dispatch->handler(req);
        }
        if(strcmp(req->uri, dispatch->uri) == 0) {
            return dispatch->handler(req);
        }
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No such URI");
    return ESP_FAIL;
}

static httpd_uri_t _httpdGet = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = _httpdHandler,
    .user_ctx  = &poolState
};
static httpd_uri_t _httpdPost = {
    .uri       = "/*",
    .method    = HTTP_POST,
    .handler   = _httpdHandler,
    .user_ctx  = &poolState
};

static httpd_handle_t
_startWebServer(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP server on port '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &_httpdGet);
        httpd_register_uri_handler(server, &_httpdPost);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void
_stopWebServer(httpd_handle_t server)
{
    httpd_stop(server);
}

static void
_wifiStaStart(void * arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "STA start");
    esp_wifi_connect();
}

static void
_wifiDisconnectHandler(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
    ESP_LOGI(TAG, "Disconnected from WiFi");
    xEventGroupClearBits(_wifi_event_group, WIFI_EVENT_CONNECTED);

    httpd_handle_t * server = (httpd_handle_t *) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        _stopWebServer(*server);
        *server = NULL;
    }
}

static void
_wifiConnectHandler(void * arg, esp_event_base_t event_base,  int32_t event_id, void * event_data)
{
    ESP_LOGI(TAG, "Connected to WiFi");
    xEventGroupSetBits(_wifi_event_group, WIFI_EVENT_CONNECTED);

    httpd_handle_t * server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = _startWebServer();
    }
    pool_mdns_init();
}

static void
_connect2wifi(void)
{
    _wifi_event_group = xEventGroupCreate();

    //tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();  // init WiFi with configuration from non-volatile storage
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    static httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &_wifiStaStart, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_wifiDisconnectHandler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_wifiConnectHandler, &server));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // wait until either the connection is established
    EventBits_t bits = xEventGroupWaitBits(_wifi_event_group, WIFI_EVENT_CONNECTED, pdFALSE, pdFALSE, portMAX_DELAY);
    if (!bits) esp_restart();  // give up
}

extern "C" void
app_main()
{
    //static httpd_handle_t server = NULL;

    _initNvsFlash();

    // instantiate, init and start pool receive/transmit

    Rs485 rs485(UART_NUM_2);
    Receiver receiver;

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
		.use_ref_tick = false
    };
    rs485.begin(&uart_config, CONFIG_POOL_RS485_RXPIN, CONFIG_POOL_RS485_TXPIN, CONFIG_POOL_RS485_RTSPIN);
    poolState.begin();
    receiver.begin();
    transmitQueue.begin();

    // start httpd

    _httpdGet.user_ctx = &poolState;  // so the httpd can return the current state
    _httpdPost.user_ctx = &poolState;  // so the httpd can return the current state

    _connect2wifi();

    esp_ota_mark_app_valid_cancel_rollback();
    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
    xTaskCreate(&reset_task, "reset_task", 8192, NULL, 5, NULL);

    // loop

    while (1) {

        sysState_t sys;

        if (receiver.receive(&rs485, &sys)) {

            poolState.update(&sys);
            transmitQueue.transmit(&rs485);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

}

//xTaskCreate(loop_task, "loop_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
// UART Demo: https://github.com/espressif/esp-idf/blob/aaf12390eb14b95589acd98db5c268a2e56bb67e/examples/peripherals/uart_async_rxtxtasks/main/uart_async_rxtxtasks_main.c
