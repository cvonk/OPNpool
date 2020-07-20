#pragma once

#include <string.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef __cplusplus
extern "C" {
#endif

void ota_update_task(void * pvParameter);

#ifdef __cplusplus
}
#endif