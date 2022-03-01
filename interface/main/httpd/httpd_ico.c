/**
 * @brief HTTPd: HTTP server callback for endpoint "favicon.ico"
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020-2022, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>
#include <esp_http_server.h>

#include "httpd.h"

//static char const * const TAG = "httpd_ico";

/*
 * URI handler for incoming GET "/favicon.ico" requests.
 * Registered in `httpd_register_handlers`
 */

esp_err_t
httpd_ico(httpd_req_t * const req)
{
    //const char * resp_str = (const char *) req->user_ctx;  // send string passed in user context as response
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);

    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}
