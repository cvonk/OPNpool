/**
 * @brief HTTP endpoint root
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
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
#include "httpd_root.h"
#include "httpd_json.h"
#include "httpd_api.h"
#include "ipc.h"

#define MAX_CONTENT_LEN (2048)

// static const char * TAG = "httpd";

typedef struct {
    char * uri;
    esp_err_t (*handler)(httpd_req_t * req);
} uri_dispatch_t;

static uri_dispatch_t _uri_dispatch[] = {
    { (char *)"/",  httpd_root },
    { (char *)"/json",  httpd_json },
    { (char *)"/api/*",  httpd_api }
};

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

    //char const * const content = _get_content(req); free(content);

    uri_dispatch_t * dispatch = _uri_dispatch;
    for (int ii = 0; ii < ARRAY_SIZE(_uri_dispatch); ii++, dispatch++) {
        int const len = strlen(dispatch->uri);
        if (dispatch->uri[len-1] == '*' && strncmp(req->uri, dispatch->uri, len-1) == 0 ) {
            return dispatch->handler(req);
        }
        if(strcmp(req->uri, dispatch->uri) == 0) {
            return dispatch->handler(req);
        }
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No such URI");
    return ESP_FAIL;
}
