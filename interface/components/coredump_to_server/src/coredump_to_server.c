/**
  * @brief coredump, sends coredump from flash to server
 **/
// Copyright © 2020, Coert Vonk
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <esp_log.h>
#include <esp_attr.h>

#include <esp_core_dump.h>
#include <esp_flash.h>
#include <esp_log.h>
#include <esp_spi_flash.h>
#include <mbedtls/base64.h>

#include "coredump_to_server.h"

#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
static char const * const  TAG = "coredump";

// from https://github.com/espressif/esp-idf/blob/cf056a7d0b90261923b8207f21dc270313b67456/components/espcoredump/src/core_dump_uart.c
static void
esp_core_dump_b64_encode(const uint8_t *src, uint32_t src_len, uint8_t *dst) {
    const static DRAM_ATTR char b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i, j, a, b, c;

    for (i = j = 0; i < src_len; i += 3) {
        a = src[i];
        b = i + 1 >= src_len ? 0 : src[i + 1];
        c = i + 2 >= src_len ? 0 : src[i + 2];

        dst[j++] = b64[a >> 2];
        dst[j++] = b64[((a & 3) << 4) | (b >> 4)];
        if (i + 1 < src_len) {
            dst[j++] = b64[(b & 0x0F) << 2 | (c >> 6)];
        }
        if (i + 2 < src_len) {
            dst[j++] = b64[c & 0x3F];
        }
    }
    while (j % 4 != 0) {
        dst[j++] = '=';
    }
    dst[j++] = '\0';
}

esp_err_t
coredump_to_server(coredump_to_server_config_t const * const write_cfg)
{
    // tested on master branch as of 2020-07-15 (v4.3-dev-472-gcf056a7d0)
    size_t coredump_addr;
    size_t coredump_size;
    esp_err_t err = esp_core_dump_image_get(&coredump_addr, &coredump_size);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Coredump no image");
        return err;
    }
    size_t const chunk_len = 3 * 16;  // must be multiple of 3
    size_t const b64_len = chunk_len / 3 * 4 + 4;
    uint8_t * const chunk = malloc(chunk_len);
    char * const b64 = malloc(b64_len);
    assert(chunk && b64);

    if (write_cfg->start) {
        if ((err = write_cfg->start(write_cfg->priv)) != ESP_OK) {
            return err;
        }
    }

    //ESP_LOGI(TAG, "Coredump is %u bytes", coredump_size);
    for (size_t offset = 0; offset < coredump_size; offset += chunk_len) {

        uint const read_len = MIN(chunk_len, coredump_size - offset);
        if (esp_flash_read(esp_flash_default_chip, chunk, coredump_addr + offset , read_len)) {
            ESP_LOGE(TAG, "Coredump read failed");
            break;
        }
#if 1
        esp_core_dump_b64_encode(chunk, read_len, (uint8_t *)b64);
#else
        size_t olen;
        if (mbedtls_base64_encode(b64, b64_len, &olen, chunk, read_len) != 0) {
            ESP_LOGE(TAG, "Coredump b64 failed");
            break;
        }
#endif
        //ets_printf("%s\r\n", b64);
        if (write_cfg->write) {
            if ((err = write_cfg->write(write_cfg->priv, b64)) != ESP_OK) {
                break;
            }
        }
    }
    free(chunk);
    free(b64);

    if (write_cfg->end) {
        if ((err = write_cfg->end(write_cfg->priv)) != ESP_OK) {
            return err;
        }
    }
    uint32_t sec_num = coredump_size / SPI_FLASH_SEC_SIZE;
    if (coredump_size % SPI_FLASH_SEC_SIZE) {
        sec_num++;
    }
    err = esp_flash_erase_region(esp_flash_default_chip, coredump_addr, sec_num * SPI_FLASH_SEC_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase coredump (%d)!", err);
    }
    return err;
}
