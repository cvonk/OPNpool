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
_jsonProcessQueryVars(httpd_req_t * req, char * const buf, ipc_t const * const ipc)
{
    char * callback = NULL;  // uses JSON-P to get around CORS
    char * rest = NULL;
    for (char * key = strtok_r(buf, "&", &rest); key != NULL; key = strtok_r(NULL, "&", &rest)) {
        char * value = strchr(key, '=');
        if (value) {
            *value++ = '\0';  // split key/value
            if (strcmp(key, "_") == 0) {
                ; // ignore
            } else if (strcmp(key, "callback") == 0) {
                callback = strdup(value);
                assert( callback );
            } else {
                char const * const key_dec = httpd_urldecode(key);
                if (CONFIG_POOL_DBGLVL_HTTPD > 1) {
                    ESP_LOGI(TAG, "rx query var => queueing (\"%s\": \"%s\")", key_dec, value);
                }
                ipc_send_to_pool(IPC_TO_POOL_TYP_SET, key_dec, strlen(key_dec), value, strlen(value), ipc);
                free((void *) key_dec);
            }
        }
    }
    return callback;
}

esp_err_t
httpd_json(httpd_req_t * const req)
{
    ipc_t const * const ipc = (ipc_t const * const) req->user_ctx;  // send string passed in user context as response

    // process query vars

    char * callback = NULL;  // uses JSON-P to get around CORS
    size_t const buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char * buf = (char *) malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            callback = _jsonProcessQueryVars(req, buf, ipc);
        }
        free(buf);
    }

    // respond with the pool state in JSON

    httpd_resp_set_type(req, "application/json");
    poolstate_t state;
    if (poolstate_get(&state) == ESP_OK) {
        char const * json = poolstate_to_json(&state, POOLSTATE_ELEM_TYP_ALL);
        assert(json);
        char * resp;
        if (callback) {
            assert( asprintf( &resp, "%s(%s)", callback,  json) >= 0 );
            free(callback);
        } else {
            assert( asprintf( &resp, "%s", json) >= 0 );
        }
        if (CONFIG_POOL_DBGLVL_HTTPD > 1) {
            ESP_LOGI(TAG, "tx \"%s\"", resp);
        }
        free((void *)json);
        httpd_resp_send(req, resp, strlen(resp));
        free(resp);
    } else {
        char const * const resp = "{\"error\":\"no state\"}";
        httpd_resp_send(req, resp, strlen(resp));
    }
    return ESP_OK;
}
