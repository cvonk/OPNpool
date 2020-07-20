/**
 * @brief HTTP endpoint for Google Push notifications (triggered by calander event changes)
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Sander and Coert Vonk
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

#include "http_post_server.h"
#include "ipc_msgs.h"

#define MAX_CONTENT_LEN (2048)
#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

static const char * TAG = "https_server_task";

esp_err_t
_httpdPushHandler(httpd_req_t * req)
{
    ipc_t const * const ipc = req->user_ctx;

    if (req->content_len >= MAX_CONTENT_LEN) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long");
        return ESP_FAIL;
    }
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
    uint const grs_len = 10;
    char grs[grs_len];
    bool const pushAck = httpd_req_get_hdr_value_str(req, "X-Goog-Resource-State", grs, grs_len) == ESP_OK && strcmp(grs, "sync") == 0;
    if (!pushAck) {
        buf[req->content_len] = '\0';
        ESP_LOGI(TAG, "Google push notification");

        sendToClient(TO_CLIENT_MSGTYPE_TRIGGER, buf, ipc);
        sendToMqtt(TO_MQTT_MSGTYPE_PUSH, "{ \"response\": \"pushed by Google\" }", ipc);

    }
    free(buf);
    httpd_resp_sendstr(req, "thank you");
    return ESP_OK;
}

void
http_post_server_stop(httpd_handle_t server)
{
    httpd_stop(server);
}

httpd_handle_t
http_post_server_start(ipc_t const * const ipc)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting HTTP server on port '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Server failed to start");
        return NULL;
    }

    ESP_LOGI(TAG, "Registering URI handler(s)");
    httpd_uri_t httpd_push_uri = {
        .uri = "/api/push",
        .method = HTTP_POST,
        .handler = _httpdPushHandler,
        .user_ctx = (void *)ipc
    };
    httpd_register_uri_handler(server, &httpd_push_uri);
    return server;
}