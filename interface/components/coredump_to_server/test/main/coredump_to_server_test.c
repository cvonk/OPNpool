/**
  * @brief demonstrates the use of "coredump_to_server"
  *
  * When the GPIO#0 is pulled low for 1 second, it triggers a "coredump
  * to flash".  On the WROVER-KIT, GPIO#0 is hooked up to the BOOT button.
  *
  * Upon boot, the device checks the flash memory to see if it contains a
  * coredump.  If so, the coredump is printed in base64 to UART.
  * The `idf monitor` will catch this, and call `espcoredump.py` to
  * analyze the coredump.
  *
  * For deployed devices, the coredump can be sent to e.g.. a
  * netdump server for analysis.  To do so, modify the callback functions
  * registered in `coredump_cfg`.
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
#include <driver/gpio.h>
#include <coredump_to_server.h>

#define RESET_PIN (0)
#define RESET_SECONDS (1)

static char const * const TAG = "coredump_to_server_test";

void IRAM_ATTR
_button_isr_handler(void * arg)
{
    static int64_t start = 0;
    if (gpio_get_level(RESET_PIN) == 0) {
        start = esp_timer_get_time();
    } else {
        if (esp_timer_get_time() - start > RESET_SECONDS * 1000L * 1000L) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR((TaskHandle_t)arg, &xHigherPriorityTaskWoken);
        }
    }
}

static esp_err_t
_coredump_to_server_begin_cb(void * priv)
{
    ets_printf("================= CORE DUMP START =================\r\n");
    return ESP_OK;
}

static esp_err_t
_coredump_to_server_end_cb(void * priv)
{
    ets_printf("================= CORE DUMP END ===================\r\n");
    return ESP_OK;
}

static esp_err_t
_coredump_to_server_write_cb(void * priv, char const * const str)
{
    ets_printf("%s\r\n", str);
    return ESP_OK;
}

void
app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    coredump_to_server_config_t coredump_cfg = {
        .start = _coredump_to_server_begin_cb,
        .end = _coredump_to_server_end_cb,
        .write = _coredump_to_server_write_cb,
        .priv = NULL,
    };
    coredump_to_server(&coredump_cfg);

    gpio_pad_select_gpio(RESET_PIN);
    gpio_set_direction(RESET_PIN, GPIO_MODE_INPUT);
    gpio_set_intr_type(RESET_PIN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(RESET_PIN, _button_isr_handler, xTaskGetCurrentTaskHandle());

    ESP_LOGI(TAG, "waiting for BOOT/RESET button ..");
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        assert(0);
    }
}