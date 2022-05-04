/**
 * @brief OPNpool - HTTPd: HTTP server callback for endpoint "/"
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
 * SPDX-FileCopyrightText: Copyright 2014,2019,2022 Coert Vonk
 */

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "httpd.h"
#include "../ipc/ipc.h"

//static char const * const TAG = "httpd_root";

/*
 * URI handler for incoming GET "/" requests.
 * Registered in `httpd_register_handlers`
 */

esp_err_t
httpd_root(httpd_req_t * const req)
{
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    //const char * resp_str = (const char *) req->user_ctx;  // send string passed in user context as response
    static const char * resp_str = 
        "<html>"
            "<head>"
                "<meta charset='utf-8' />"
                "<title>OPNpool Web UI</title>"
                "<link rel='shortcut icon' href='//coertvonk.com/cvonk/opnpool/favicon.ico' type='image/x-icon' />"
                "<script src='https://ajax.googleapis.com/ajax/libs/jquery/2.1.4/jquery.min.js'></script>"
                "<link rel='stylesheet' href='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css'>"
                "<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/round-slider@1.6.1/dist/roundslider.min.css'>"
                "<script src='https://cdn.jsdelivr.net/npm/round-slider@1.6.1/dist/roundslider.min.js'></script>"
                "<link rel='stylesheet' href='http://coertvonk.com/cvonk/opnpool/index.css'>"
                "<script src='http://coertvonk.com/cvonk/opnpool/replace-body.js'></script>"
            "</head>"
            "<body>"
                "<h1>Page loading</h1>"
            "</body>"
        "</html>";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}
