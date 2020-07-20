/**
 * @brief HTTP endpoint root
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_ota_ops.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_tls.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "httpd_root.h"
#include "ipc_msgs.h"

#define MAX_CONTENT_LEN (2048)

static const char * TAG = "httpd_root";

esp_err_t
httpd_root_cb(httpd_req_t * req)
{
    ipc_t const * const ipc = req->user_ctx;

    if (req->content_len >= MAX_CONTENT_LEN) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long");
        return ESP_FAIL;
    }
#if 1
    const char * resp_str = "<html>\n"
        "<head>\n"
        "</head>\n"
        "<body>\n"
        "  <h1>Welcome to device world</h1>\n"
        "</body>\n"
        "</html>";
#else
    // +1 so even for no content body, we have a buf
    char * const buf = malloc(req->content_len + 1);  // https_client_task reads from Q and frees mem
    assert(buf);

    uint len = 0;
    while (len < req->content_len) {
        uint received = httpd_req_recv(req, buf + len, req->content_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post value");
            return ESP_FAIL;
        }
        len += received;
    }

    buf[req->content_len] = '\0';
    free(buf);
#endif

    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}
