/**
  * @brief demonstrates the use of "ota_update_task"
  *
  * Upon boot, the device checks for an OTA update, and downloads
  * it and restarts.
 **/
// Copyright © 2020, Coert Vonk
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <ota_update_task.h>

#define WIFI_DEVIPADDR_LEN (16)
static char const * const TAG = "ota_update_test";

static EventGroupHandle_t _wifi_event_group;
typedef enum {
    WIFI_EVENT_CONNECTED = BIT0
} my_wifi_event_t;

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

static void
_wifiStaStart(void * arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "STA start => connect to WiFi AP");
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void
_wifiDisconnectHandler(void * arg_void, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
    ESP_LOGI(TAG, "WiFi disconnected");
    xEventGroupClearBits(_wifi_event_group, WIFI_EVENT_CONNECTED);

    vTaskDelay(10000L / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void
_wifiConnectHandler(void * arg_void, esp_event_base_t event_base,  int32_t event_id, void * event_data)
{
    xEventGroupSetBits(_wifi_event_group, WIFI_EVENT_CONNECTED);

    char * const ipAddr = arg_void;
    ip_event_got_ip_t const * const event = (ip_event_got_ip_t *) event_data;
    snprintf(ipAddr, WIFI_DEVIPADDR_LEN, IPSTR, IP2STR(&event->ip_info.ip));
}

static void
_connect2wifi(char * const ipAddr)
{
    _wifi_event_group = xEventGroupCreate();
    assert(_wifi_event_group);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();  // init WiFi with configuration from non-volatile storage
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &_wifiStaStart, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_wifiDisconnectHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_wifiConnectHandler, ipAddr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    if (strlen(CONFIG_WIFI_SSID)) {
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = CONFIG_WIFI_SSID,
                .password = CONFIG_WIFI_PASSWD
            }
        };
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    };
    ESP_ERROR_CHECK(esp_wifi_start());

    // wait until either the connection is established
    assert(xEventGroupWaitBits(_wifi_event_group, WIFI_EVENT_CONNECTED, pdFALSE, pdFALSE, portMAX_DELAY));
}

void
app_main()
{
    _initNvsFlash();

    char ipAddr[WIFI_DEVIPADDR_LEN];
    _connect2wifi(ipAddr);  // waits for connection established
    ESP_LOGI(TAG, "IPv4 addr = %s", ipAddr);

    xTaskCreate(&ota_update_task, "ota_update_task", 4096, NULL, 5, NULL);
}