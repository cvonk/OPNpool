/**
 * @brief OPNpool - HTTPd: HTTP server callback for endpoint "/who"
 *
 * © Copyright 2014, 2019, 2022, Coert Vonk
 * 
 * This file is part of OPNpool.
 * OPNpool is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * OPNpool is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with OPNpool. 
 * If not, see <https://www.gnu.org/licenses/>.
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_ota_ops.h>
#include <esp_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "httpd.h"
#include "../ipc/ipc.h"

//static char const * const TAG = "httpd_who";

/*
 * URI handler for incoming GET "/json" requests.
 * Registered in `httpd_register_handlers`
 */

esp_err_t
httpd_who(httpd_req_t * const req)
{
    ipc_t const * const ipc = (ipc_t const * const) req->user_ctx;  // send string passed in user context as response

    // respond with the pool state in JSON

    httpd_resp_set_type(req, "application/json");

    esp_partition_t const * const running_part = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    ESP_ERROR_CHECK(esp_ota_get_partition_description(running_part, &running_app_info));

    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));

    char * resp;
    assert( asprintf(&resp,
            "{ \"name\": \"%s\", \"firmware\": { \"version\": \"%s.%s\", \"date\": \"%s %s\" }, \"wifi\": { \"connect\": %u, \"address\": \"%s\", \"SSID\": \"%s\", \"RSSI\": %d }, \"mqtt\": { \"connect\": %u }, \"mem\": { \"heap\": %u } }",
            ipc->dev.name,
            running_app_info.project_name, running_app_info.version,
            running_app_info.date, running_app_info.time,
            ipc->dev.count.wifiConnect, ipc->dev.ipAddr, ap_info.ssid, ap_info.rssi,
            ipc->dev.count.mqttConnect, heap_caps_get_free_size(MALLOC_CAP_8BIT) ) >= 0);
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);

    return ESP_OK;
}
