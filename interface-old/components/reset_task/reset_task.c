/* Reset when running and BOOT/RESET button is pressed for 3 seconds
*/
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <driver/gpio.h>

#include "reset_task.h"

static char const * const TAG = "reset_task";

void IRAM_ATTR _button_isr_handler(void * arg)  {

    static int64_t start = 0;
    if (gpio_get_level(CONFIG_RESET_PIN) == 0) {
        start = esp_timer_get_time();
    } else {
        if (esp_timer_get_time() - start > CONFIG_RESET_SECONDS * 1000L * 1000L) {
            xSemaphoreGiveFromISR(*(SemaphoreHandle_t *)arg, NULL);
        }
    }
}

void reset_task(void * pvParameter) {

    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();

    gpio_pad_select_gpio(CONFIG_RESET_PIN);
    gpio_set_direction(CONFIG_RESET_PIN, GPIO_MODE_INPUT);
    gpio_set_intr_type(CONFIG_RESET_PIN, GPIO_INTR_ANYEDGE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(CONFIG_RESET_PIN, _button_isr_handler, &semaphore);

    ESP_LOGI(TAG, "waiting for BOOT/RESET button ..");

    while (1) {
        if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {
            ESP_LOGW(TAG, "Factory reset ..");
            esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
            if (pi) {
                esp_partition_t const * factory = esp_partition_get(pi);
                esp_partition_iterator_release(pi);
                if (esp_ota_set_boot_partition(factory) == ESP_OK) {
                    esp_wifi_restore();  // erase stored WiFI SSID/passwd
                    esp_restart();
                }
            } else {
                ESP_LOGE(TAG, "No factory partition");
            }
        }
    }
}
