/**
 * @brief HTTPd: HTTP server callback for endpoint "/"
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020-2022, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

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
                "<title>Pool webapp</title>"
                "<link rel='shortcut icon' href='//coertvonk.com/cvonk/pool/favicon.ico' type='image/x-icon' />"
                "<script src='https://ajax.googleapis.com/ajax/libs/jquery/2.1.4/jquery.min.js'></script>"
                "<link rel='stylesheet' href='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css'>"
                // "<script src='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js'></script>"
                "<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/round-slider@1.6.1/dist/roundslider.min.css'>"
                "<script src='https://cdn.jsdelivr.net/npm/round-slider@1.6.1/dist/roundslider.min.js'></script>"
                "<link rel='stylesheet' href='http://coertvonk.com/cvonk/pool/index.css'>"
                "<script src='http://coertvonk.com/cvonk/pool/replace-body.js'></script>"
            "</head>"
            "<body>"
                "<h1>Page loading</h1>"
            "</body>"
        "</html>";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}
