/**
 * @brief HTTPd: HTTP server; started in response to receiving an IP address; http_uries to httpd_*.c callbacks
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_http_server.h>

#include "httpd.h"
#include "../ipc/ipc.h"

static char const * const TAG = "httpd";

static httpd_uri_t _httpd_uris[] = {
    { .uri = "/",      .method = HTTP_GET, .handler = httpd_root },
    { .uri = "/json",  .method = HTTP_GET, .handler = httpd_json },
};

void
httpd_register_cb(httpd_handle_t const httpd_handle, esp_ip4_addr_t const * const ip, ipc_t const * const ipc)
{
    httpd_uri_t * http_uri = _httpd_uris;
    for (int ii = 0; ii < ARRAY_SIZE(_httpd_uris); ii++, http_uri++) {

        http_uri->user_ctx = (void *) ipc;
        ESP_ERROR_CHECK( httpd_register_uri_handler(httpd_handle, http_uri) );
        ESP_LOGI(TAG, "Listening at http://" IPSTR "/%s", IP2STR(ip), http_uri->uri);
    }
}

#if 0
#define MAX_CONTENT_LEN (2048)

char const *
httpd_get_content(httpd_req_t * const req)
{
    if (req->content_len >= MAX_CONTENT_LEN) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long");
        return NULL;
    }
    char * const buf = malloc(req->content_len + 1);  // +1 so even for no content body, we have a buf
    assert(buf);
    uint len = 0;
    while (len < req->content_len) {
        uint received = httpd_req_recv(req, buf + len, req->content_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post value");
            return NULL;
        }
        len += received;
    }
    buf[req->content_len] = '\0';
    return buf; // buf must be freed by caller
}

esp_err_t
httpd_cb(httpd_req_t * const req)
{
    //ipc_t const * const ipc = req->user_ctx;

    if (req->method != HTTP_POST && req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Request for \"%s\"", req->uri);
    http_uri_t const * http_uri = _httpd_uris;
    for (int ii = 0; ii < ARRAY_SIZE(_httpd_uris); ii++, http_uri++) {
        int const len = strlen(http_uri->uri);
        if (http_uri->uri[len-1] == '*' && strncmp(req->uri, http_uri->uri, len-1) == 0 ) {
            return http_uri->fnc(req);
        }
        ESP_LOGI(TAG, "2 \"%s\" == \"%s\"", req->uri, http_uri->uri);
        if(strcmp(req->uri, http_uri->uri) == 0) {
            return http_uri->fnc(req);
        }
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No such URI");
    return ESP_FAIL;
}
#endif