/* based on PD/CC0 licensed https://github.com/espressif/esp-idf/blob/master/examples/system/ota/native_ota_example/main/native_ota_example.c

   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_image_format.h"

#include "ota_task.h"
#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */

static char const * const TAG = "ota_task";
extern uint8_t const server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern uint8_t const server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
static char ota_write_data[BUFFSIZE + 1] = { 0 };  // OTA data buffer ready to write to flash

static void
_http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void __attribute__((noreturn))
_delete_task()
{
    ESP_LOGI(TAG, "Exiting task ..");
    (void)vTaskDelete(NULL);

    while (1) {  // FreeRTOS requires that tasks never return
        ;
    }
}

static void
_infinite_loop(void)
{
    ESP_LOGI(TAG, "When a new firmware is available on the server, press the reset button to download it");
    _delete_task();
}

static bool
_versions_match(esp_app_desc_t const * const desc1, esp_app_desc_t const * const desc2)
{
    return
        memcmp(desc1->project_name, desc2->project_name, sizeof(desc1->project_name)) == 0 &&
        memcmp(desc1->version, desc2->version, sizeof(desc1->version)) == 0 &&
        memcmp(desc1->date, desc2->date, sizeof(desc1->date)) == 0 &&
        memcmp(desc1->time, desc2->time, sizeof(desc1->time)) == 0;
}

void
ota_task(void * pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA (%s)", CONFIG_FIRMWARE_UPGRADE_URL);

    esp_partition_t const * const configured_part = esp_ota_get_boot_partition();
    esp_partition_t const * const running_part = esp_ota_get_running_partition();
    esp_partition_t const * update_part = esp_ota_get_next_update_partition(NULL);

    if (configured_part != running_part) {
        ESP_LOGW(TAG, "Either the OTA boot data or preferred boot image is corrupted (0x%08x 0x%08x)", configured_part->address, running_part->address);
    }
    ESP_LOGI(TAG, "Running partition %s at offset 0x%08x", running_part->label, running_part->address);

    esp_http_client_config_t config = {
        .url = CONFIG_FIRMWARE_UPGRADE_URL,
        //.cert_pem = (char *)server_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init connection to server");
        _delete_task();
    }
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open connection to server (%s)", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        _delete_task();
    }
    esp_http_client_fetch_headers(client);
    if (esp_http_client_get_status_code(client) != 200) {
        ESP_LOGE(TAG, "File doesn't exist on server (%s), status=%d", CONFIG_FIRMWARE_UPGRADE_URL, esp_http_client_get_status_code(client));
        esp_http_client_cleanup(client);
        _delete_task();
    }

    ESP_LOGI(TAG, "Writing to partition %s at offset 0x%x", update_part->label, update_part->address);
    assert(update_part != NULL);

    int binary_file_length = 0;
    bool image_header_was_checked = false;
    esp_ota_handle_t update_handle = 0 ;  // set by esp_ota_begin(), must be freed via esp_ota_end()

    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "SSL data read error");
            _http_cleanup(client);
            _delete_task();
        } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {

                    // 2BD test to see if this is firmware, or just a place holder page

                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "Firmware on server: %s.%s (%s %s)",
                        new_app_info.project_name, new_app_info.version, new_app_info.date, new_app_info.time);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running_part, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Firmware running:   %s.%s (%s %s)",
                            running_app_info.project_name, running_app_info.version, running_app_info.date, running_app_info.time);
                    }
                    esp_partition_t const * const last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Firmware marked invalid: %s.%s (%s %s)",
                            invalid_app_info.project_name, invalid_app_info.version, invalid_app_info.date, invalid_app_info_info.time);
                    }
                    if (last_invalid_app != NULL) {
                        if (_versions_match(&invalid_app_info, &new_app_info)) {
                            ESP_LOGW(TAG, "Version on server is the same as invalid version (%s)", invalid_app_info.version);
                            _http_cleanup(client);
                            _infinite_loop();
                        }
                    }
                    if (_versions_match(&new_app_info, &running_app_info)) {
                        ESP_LOGW(TAG, "No update available");
                        _http_cleanup(client);
                        _infinite_loop();
                    }
                    ESP_LOGW(TAG, "Downloading OTA update");
                    image_header_was_checked = true;

                    esp_err_t const err = esp_ota_begin(update_part, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        _http_cleanup(client);
                        _delete_task();
                    }
                } else {
                    ESP_LOGE(TAG, "rx package len err");
                    _http_cleanup(client);
                    _delete_task();
                }
            }
            if (esp_ota_write( update_handle, (const void *)ota_write_data, data_read) != ESP_OK) {
                ESP_LOGE(TAG, "OTA write err, is partition large enough?");
                _http_cleanup(client);
                _delete_task();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Wrote %d bytes", binary_file_length);
        } else if (data_read == 0) {
            ESP_LOGI(TAG, "OTA finished");
            break;
        }
    }
    ESP_LOGI(TAG, "Finished, wrote %d bytes", binary_file_length);

    if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        _http_cleanup(client);
        _delete_task();
    }

    err = esp_ota_set_boot_partition(update_part);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        _http_cleanup(client);
        _delete_task();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
    return ;
}
