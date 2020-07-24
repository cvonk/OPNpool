/**
 * @brief HTTP endpoint "/api"
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
#include "httpd_api.h"
#include "ipc.h"

static char const * const TAG = "httpd_api";

typedef struct {
    char * uri;
    char * key;
} api_variable_t;

static api_variable_t _api_variables[] = {
    {(char *)"/api/circuit/pool", (char *)"pool-active"},
    {(char *)"/api/circuit/aux1", (char *)"aux1-active"},
    {(char *)"/api/pool/sp", (char *)"pool-sp"},
    {(char *)"/api/pool/src", (char *)"pool-src"},
    {(char *)"/api/spa/sp", (char *)"spa-sp"},
    {(char *)"/api/spa/src", (char *)"spa-src"},
};

esp_err_t
httpd_api(httpd_req_t * const req)
{
    if (req->method != HTTP_POST && req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    api_variable_t * variable = _api_variables;
    for (int ii = 0; ii < ARRAY_SIZE(_api_variables); ii++, variable++) {
        if (strcmp(req->uri, variable->uri) == 0 ) {

            if (req->method == HTTP_POST) {
                char value[16];  // body of POST message holds the new value
                size_t recv_size = MIN(req->content_len, sizeof(value));
                int ret = httpd_req_recv(req, value, recv_size);
                if (ret <= 0) {
                    if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                        httpd_resp_send_408(req);  // KISS don't retry, just give up
                    }
                    return ESP_FAIL;
                }
                value[recv_size] = '\0';
                ESP_LOGI(TAG, "API POST var: %s=%s", variable->key, value);
#if 0
                transmitQueue.enqueue(variable->key, value);  // MAKE SURE that the queue copies the values, as they will be freed soon
#endif
            }
            ESP_LOGI(TAG, "API GET var: %s", variable->key);
            char reply[40];
#if 0
            PoolState * const state = (PoolState *) req->user_ctx;
            if( strcmp(req->uri, "/api/circuit/pool") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": %s }",  state->getCircuitState("pool") ? "true" : "false");
            }
            else if( strcmp(req->uri, "/api/circuit/aux1") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": %s }",  state->getCircuitState("aux1") ? "true" : "false");
            }
            else if( strcmp(req->uri, "/api/pool/sp") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": %d }",  state->getSetPoint("pool"));
            }
            else if( strcmp(req->uri, "/api/pool/src") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": \"%s\" }",  state->getHeatSrc("pool"));
            }
            else if( strcmp(req->uri, "/api/spa/sp") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": %d }",  state->getSetPoint("spa"));
            }
            else if( strcmp(req->uri, "/api/spa/src") == 0) {
                snprintf(reply, sizeof(reply), "{ \"value\": \"%s\" }",  state->getHeatSrc("spa"));
            }
            else {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No such API");
                return ESP_FAIL;
            }
#else
            reply[0] = '\0';
#endif
            ESP_LOGI(TAG, "API reply %s(%d) %d", reply, strlen(reply), sizeof(reply));
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, reply, strlen(reply));
            return ESP_OK;
        }
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No such API");
    return ESP_FAIL;
}

