/**
* @brief mqtt_client_task, interface with the MQTT broker
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <mqtt_client.h>
#include <esp_ota_ops.h>
#include <esp_core_dump.h>
#include <esp_flash.h>

#include "mqtt_task.h"
#include "ipc.h"
#include "coredump_to_server.h"

static char const * const TAG = "mqtt_task";

static EventGroupHandle_t _mqttEventGrp = NULL;
typedef enum {
	MQTT_EVENT_CONNECTED_BIT = BIT0
} mqttEvent_t;

static struct {
    char * ctrl;
    char * ctrlGroup;
} _topic;

typedef struct coredump_priv_t {
    esp_mqtt_client_handle_t const client;
    char * topic;
} coredump_priv_t;

static esp_mqtt_client_handle_t _connect2broker(ipc_t const * const ipc);  // forward decl

static esp_err_t
_coredump_to_server_write_cb(void * priv_void, char const * const str)
{
    coredump_priv_t const * const priv = priv_void;

    esp_mqtt_client_publish(priv->client, priv->topic, str, strlen(str), 1, 0);
    return ESP_OK;
}

static void
_forwardCoredump(ipc_t * ipc, esp_mqtt_client_handle_t const client)
{
    coredump_priv_t priv = {
        .client = client,
    };
    asprintf(&priv.topic, "%s/coredump/%s", CONFIG_POOL_MQTT_DATA_TOPIC, ipc->dev.name);
    coredump_to_server_config_t coredump_cfg = {
        .start = NULL,
        .end = NULL,
        .write = _coredump_to_server_write_cb,
        .priv = &priv,
    };
    coredump_to_server(&coredump_cfg);
    free(priv.topic);
}

static esp_err_t
_mqttEventHandler(esp_mqtt_event_handle_t event) {

    ipc_t * const ipc = event->user_context;

	switch (event->event_id) {
        case MQTT_EVENT_DISCONNECTED:
            xEventGroupClearBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT);
            ESP_LOGW(TAG, "Broker disconnected");
        	// reconnect is part of the SDK
            break;
        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT);
            ipc->dev.count.mqttConnect++;
            esp_mqtt_client_subscribe(event->client, _topic.ctrl, 1);
            esp_mqtt_client_subscribe(event->client, _topic.ctrlGroup, 1);
            ESP_LOGI(TAG, "Broker connected, subscribed to \"%s\", \"%s\"", _topic.ctrl, _topic.ctrlGroup);
            break;
        case MQTT_EVENT_DATA:
            if (event->topic && event->data_len == event->total_data_len) {  // quietly ignore chunked messaegs

                if (strncmp("restart", event->data, event->data_len) == 0) {

                    sendToMqtt(IPC_TO_MQTT_TYP_RESTART, "{ \"response\": \"restarting\" }", ipc);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    esp_restart();

                } else if (strncmp("who", event->data, event->data_len) == 0) {

                    esp_partition_t const * const running_part = esp_ota_get_running_partition();
                    esp_app_desc_t running_app_info;
                    ESP_ERROR_CHECK(esp_ota_get_partition_description(running_part, &running_app_info));

                    wifi_ap_record_t ap_info;
                    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));

                    char * payload;
                    int const payload_len = asprintf(&payload,
                        "{ \"name\": \"%s\", \"firmware\": { \"version\": \"%s.%s\", \"date\": \"%s %s\" }, \"wifi\": { \"connect\": %u, \"address\": \"%s\", \"SSID\": \"%s\", \"RSSI\": %d }, \"mqtt\": { \"connect\": %u }, \"mem\": { \"heap\": %u } }",
                        ipc->dev.name,
                        running_app_info.project_name, running_app_info.version,
                        running_app_info.date, running_app_info.time,
                        ipc->dev.count.wifiConnect, ipc->dev.ipAddr, ap_info.ssid, ap_info.rssi,
                        ipc->dev.count.mqttConnect, heap_caps_get_free_size(MALLOC_CAP_8BIT)
                    );
                    assert(payload_len >= 0);
                    sendToMqtt(IPC_TO_MQTT_TYP_WHO, payload, ipc);
                    free(payload);
                }
            }
            break;
        default:
            break;
	}
	return ESP_OK;
}

static esp_mqtt_client_handle_t
_connect2broker(ipc_t const * const ipc) {

    esp_mqtt_client_handle_t client;
    xEventGroupClearBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT);
    {
        const esp_mqtt_client_config_t mqtt_cfg = {
            .event_handle = _mqttEventHandler,
            .user_context = (void *)ipc,
            .uri = CONFIG_POOL_MQTT_URL,
        };
        client = esp_mqtt_client_init(&mqtt_cfg);
        //ESP_ERROR_CHECK(esp_mqtt_client_set_uri(client, CONFIG_POOL_MQTT_URL));
        ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    }
	assert(xEventGroupWaitBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY));
    return client;
}

void
mqtt_task(void * ipc_void)
{
 	ipc_t * ipc = ipc_void;

    _topic.ctrl = malloc(strlen(CONFIG_POOL_MQTT_CTRL_TOPIC) + 1 + strlen(ipc->dev.name) + 1);
    _topic.ctrlGroup = malloc(strlen(CONFIG_POOL_MQTT_CTRL_TOPIC) + 1);
    assert(_topic.ctrl && _topic.ctrlGroup);
    sprintf(_topic.ctrl, "%s/%s", CONFIG_POOL_MQTT_CTRL_TOPIC, ipc->dev.name);
    sprintf(_topic.ctrlGroup, "%s", CONFIG_POOL_MQTT_CTRL_TOPIC);

	_mqttEventGrp = xEventGroupCreate();
    esp_mqtt_client_handle_t const client = _connect2broker(ipc);

    _forwardCoredump(ipc, client);

	while (1) {
        toMqttMsg_t msg;
		if (xQueueReceive(ipc->to_mqtt_q, &msg, (TickType_t)(1000L / portTICK_PERIOD_MS)) == pdPASS) {

            char * topic;
            char const * const subtopic = ipc_to_mqtt_typ_str(msg.dataType);
            if (subtopic) {
                asprintf(&topic, "%s/%s/%s", CONFIG_POOL_MQTT_DATA_TOPIC, subtopic, ipc->dev.name);
            } else {
                asprintf(&topic, "%s/%s", CONFIG_POOL_MQTT_DATA_TOPIC, ipc->dev.name);
            }
            esp_mqtt_client_publish(client, topic, msg.data, strlen(msg.data), 1, 0);
            free(topic);
            free(msg.data);
		}
	}
}