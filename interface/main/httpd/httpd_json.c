/**
 * @brief HTTPd: HTTP server callback for endpoint "/json"
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
#include "../utils/utils.h"
#include "../ipc/ipc.h"
#include "../poolstate/poolstate.h"

static char const * const TAG = "httpd_json";

static char *
_jsonProcessQueryVars(httpd_req_t * req, char * const buf)
{
    char * callback = NULL;  // uses JSON-P to get around CORS
    char * rest = NULL;
    for (char * key = strtok_r(buf, "&", &rest); key != NULL; key = strtok_r(NULL, "&", &rest)) {
        char * value = strchr(key, '=');
        if (value) {
            *value++ = '\0';
            if (strcmp(key, "callback") == 0) {
                callback = (char *) malloc(strlen(value)+1);
                if (callback) {
                    strcpy(callback, value);
                }
            }
            ESP_LOGI(TAG, "query var, %s=%s", key, value);
        }
#if 0
        transmitQueue.enqueue(key, value);  // MAKE SURE that the queue copies the values, as they will be freed soon
#endif
    }
    return callback;
}

esp_err_t
httpd_json(httpd_req_t * const req)
{
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    // process query vars

    char * callback = NULL;  // uses JSON-P to get around CORS
    size_t const buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char * buf = (char *) malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            callback = _jsonProcessQueryVars(req, buf);
        }
        free(buf);
    }

    // return pool state

    size_t len = 0;
    size_t const resp_size = 1024;
    char resp[resp_size];
    resp[0] = '\0';
    if (callback) {
        snprintf(resp + len, resp_size - len, "%s(", callback);
    }
    poolstate_t state;
    if (poolstate_get(&state) == ESP_OK) {
        len += poolstate_to_json(&state, resp + len, resp_size - len);
    } else {
        ESP_LOGW(TAG, "state not avail");
    }
    if (callback) {
        snprintf(resp + len, resp_size - len, ")");
        free(callback);
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}
