#pragma once
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#ifdef __cplusplus
extern "C" {
#endif

void reset_task(void * pvParameter);

#ifdef __cplusplus
}
#endif