/**
  * @brief demonstrates the use of "factory_reset_task"
  *
  * Erases WiFi credentials in nvram when GPIO#0 pulled low.
 **/
// Copyright © 2020, Coert Vonk
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <string.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <factory_reset_task.h>

void
app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    xTaskCreate(&factory_reset_task, "factory_reset_task", 4096, NULL, 5, NULL);
}