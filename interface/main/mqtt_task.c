/**
 * @brief OPNpool, MQTT client task: interface with the MQTT broker
 *
 * © Copyright 2014, 2019, 2022, Coert Vonk
 * 
 * This file is part of OPNpool.
 * OPNpool is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * OPNpool is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with OPNpool. 
 * If not, see <https://www.gnu.org/licenses/>.
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


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
#include <nvs_flash.h>
#include <nvs.h>

#include "ipc/ipc.h"
#include "mqtt_task.h"
#include "coredump_to_server.h"

static char const * const TAG = "mqtt_task";

static EventGroupHandle_t _mqttEventGrp = NULL;
typedef enum {
	MQTT_EVENT_CONNECTED_BIT = BIT0
} mqttEvent_t;

typedef struct coredump_priv_t {
    esp_mqtt_client_handle_t const client;
    char * topic;
} coredump_priv_t;

static esp_mqtt_client_handle_t _connect2broker(ipc_t const * const ipc);  // forward decl

/*
 * Linked list of MQTT subscribers
 */

typedef struct mqtt_subscriber_t {
    char const * topic;
    bool local;  // indicates if messages should be handled by this task itself
    struct mqtt_subscriber_t const * next;
} mqtt_subscriber_t;

mqtt_subscriber_t * _subscribers = NULL;

/*
 * Add MQTT subscriber to linked list
 */

static void
_add_subscriber(char const * const topic, bool const local)
{
    mqtt_subscriber_t * subscriber = malloc(sizeof(mqtt_subscriber_t));
    subscriber->topic = topic,
    subscriber->local = local,
    subscriber->next = _subscribers,
    _subscribers = subscriber;  // insert at HEAD of linked list
}

/*
 * Callback function to MQTT publish one line of base64 encoded message
 */

static esp_err_t
_coredump_to_server_write_cb(void * priv_void, char const * const str)
{
    coredump_priv_t const * const priv = priv_void;

    esp_mqtt_client_publish(priv->client, priv->topic, str, strlen(str), 1, 0);
    return ESP_OK;
}

/*
 * If there is a coredump in flash memory, then publish it to MQTT 
 * under topic `opnpool/data/coredump/ID` using base64 encoding.
 */

static void
_forwardCoredump(ipc_t * ipc, esp_mqtt_client_handle_t const client)
{
    coredump_priv_t priv = {
        .client = client,
    };
    assert( asprintf(&priv.topic, "%s/coredump/%s", CONFIG_OPNPOOL_MQTT_DATA_TOPIC, ipc->dev.name) >=0 );
    coredump_to_server_config_t coredump_cfg = {
        .start = NULL,
        .end = NULL,
        .write = _coredump_to_server_write_cb,
        .priv = &priv,
    };
    coredump_to_server(&coredump_cfg);
    free(priv.topic);
}

/*
 * Restart the device.
 * Called from `_mqtt_event_cb` in response the value `restart` received on the MQTT control topic.
 */

static void
_dispatch_restart(esp_mqtt_event_handle_t event, ipc_t const * const ipc)
{
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH_DATA_RESTART, "{ \"response\": \"restarting\" }", ipc);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
}

/*
 * Show information about the device, such as build version, wifi details, and mqtt status.
 * Called from `_mqtt_event_cb` in response the value `restart` received on the MQTT control topic.
 */

static void
_dispatch_who(esp_mqtt_event_handle_t event, ipc_t const * const ipc)
{
    esp_partition_t const * const running_part = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    ESP_ERROR_CHECK(esp_ota_get_partition_description(running_part, &running_app_info));

    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));

    char * payload;
    assert( asprintf(&payload,
            "{ \"name\": \"%s\", \"firmware\": { \"version\": \"%s.%s\", \"date\": \"%s %s\" }, \"wifi\": { \"connect\": %u, \"address\": \"%s\", \"SSID\": \"%s\", \"RSSI\": %d }, \"mqtt\": { \"connect\": %u }, \"mem\": { \"heap\": %u } }",
            ipc->dev.name,
            running_app_info.project_name, running_app_info.version,
            running_app_info.date, running_app_info.time,
            ipc->dev.count.wifiConnect, ipc->dev.ipAddr, ap_info.ssid, ap_info.rssi,
            ipc->dev.count.mqttConnect, heap_caps_get_free_size(MALLOC_CAP_8BIT) ) >= 0);
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH_DATA_WHO, payload, ipc);
    free(payload);
}

#if 0
/*
 * Start ESF IDF heap tracing.
 * Called from `_mqtt_event_cb` in response the value `startht` received on the MQTT control topic.
 */

static void
_heap_trace_start(esp_mqtt_event_handle_t event, ipc_t const * const ipc)
{
    ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );
}

/*
 * Stop ESF IDF heap tracing.
 * Called from `_mqtt_event_cb` in response the value `stopht` received on the MQTT control topic.
 */

static void
_heap_trace_stop(esp_mqtt_event_handle_t event, ipc_t const * const ipc)
{
    ESP_ERROR_CHECK( heap_trace_stop() );
    heap_trace_dump();
}
#endif

/*
 * Dispatch table used handled locally by this task
 */

typedef void (* mqtt_dispatch_fnc_t)(esp_mqtt_event_handle_t event, ipc_t const * const ipc);

typedef struct mqtt_dispatch_t {
    char * message;
    mqtt_dispatch_fnc_t fnc;
} mqtt_dispatch_t;

static mqtt_dispatch_t _mqtt_dispatches[] = {
    {"who", _dispatch_who},
    {"restart", _dispatch_restart},
#if 0
    {"htstart", _heap_trace_start},
    {"htstop", _heap_trace_stop},
#endif
};

/*
 * Handler for MQTT events, such as connect/disconnect from broker, or messages received on the control topic.
 * Registered through `esp_mqtt_client_init`.
 */

static esp_err_t
_mqtt_event_cb(esp_mqtt_event_handle_t event) {

    ipc_t * const ipc = event->user_context;

	switch (event->event_id) {

        case MQTT_EVENT_DISCONNECTED:  // indicates that we got disconnected from the MQTT broker

            xEventGroupClearBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT);
            if (CONFIG_OPNPOOL_DBGLVL_MQTTTASK > 0) {
                ESP_LOGW(TAG, "Broker disconnected");
            }
        	// reconnect is part of the SDK
            break;

        case MQTT_EVENT_CONNECTED:  // indicates that we're connected to the MQTT broker

            xEventGroupSetBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT);
            ipc->dev.count.mqttConnect++;

            // subscribe to the registered control topics
            for (mqtt_subscriber_t const * subscriber = _subscribers; subscriber; subscriber = subscriber->next) {
                esp_mqtt_client_subscribe(event->client, subscriber->topic, 1);
                if (CONFIG_OPNPOOL_DBGLVL_MQTTTASK > 1) {
                    ESP_LOGI(TAG, "Broker connected, subscribed to \"%s\"", subscriber->topic);
                }
            }
            break;

        case MQTT_EVENT_DATA:  // indicates that data is received on the MQTT control topic

            if (event->topic && event->data_len == event->total_data_len) {  // quietly ignore chunked messaegs
                bool done = false;

                for (mqtt_subscriber_t const * subscriber = _subscribers; subscriber && !done; subscriber = subscriber->next) {
                    if (strncmp(subscriber->topic, event->topic, event->topic_len) == 0) {

                        if (subscriber->local) {
                            // handled locally by this module
                            mqtt_dispatch_t const * handler = _mqtt_dispatches;
                            for (uint ii = 0; ii < ARRAY_SIZE(_mqtt_dispatches) && !done; ii++, handler++) {
                                if (strncmp(handler->message, event->data, event->data_len) == 0) {
                                    handler->fnc(event, ipc);
                                }
                            }
                        } else {
                            // handled by the `pool_task`
                            ipc_send_to_pool(IPC_TO_POOL_TYP_SET, event->topic, event->topic_len, event->data, event->data_len, ipc);
                        }
                        done = true;
                    }
                }
                if (!done) {  // perhaps `pool_task` knows what to do
                }
            }
            break;
        default:
            break;
	}
	return ESP_OK;
}

/*
 * Connect to MQTT broker
 * The `MQTT_EVENT_CONNECTED_BIT` in `mqttEventGrp` indicates that we're connected to the MQTT broker.
 * This bit is controlled by the `mqtt_event_cb`
 */

static esp_mqtt_client_handle_t
_connect2broker(ipc_t const * const ipc) {

    char * mqtt_url = NULL;

#ifdef CONFIG_OPNPOOL_HARDCODED_MQTT_CREDENTIALS
    ESP_LOGW(TAG, "Using mqtt_url from Kconfig");
    mqtt_url = strdup(CONFIG_OPNPOOL_HARDCODED_MQTT_URL);
#else
    ESP_LOGW(TAG, "Using mqtt_url from nvram");
    nvs_handle_t nvs_handle;
    size_t len;
    if (nvs_open("storage", NVS_READONLY, &nvs_handle) == ESP_OK &&
        nvs_get_str(nvs_handle, "mqtt_url", NULL, &len) == ESP_OK) {
            
        mqtt_url = (char *) malloc(len);
        ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "mqtt_url", mqtt_url, &len));
    }
#endif
    if (mqtt_url == NULL || *mqtt_url == '\0') {
        return NULL;
    }

    ESP_LOGW(TAG, "read mqtt_url (%s)", mqtt_url);

    esp_mqtt_client_handle_t client;
    xEventGroupClearBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT);
    {
        const esp_mqtt_client_config_t mqtt_cfg = {
            .event_handle = _mqtt_event_cb,
            .user_context = (void *) ipc,
            .uri = mqtt_url
        };
        client = esp_mqtt_client_init(&mqtt_cfg);
        //ESP_ERROR_CHECK(esp_mqtt_client_set_uri(client, CONFIG_POOL_MQTT_URL));
        ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    }
	assert(xEventGroupWaitBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY));
    free(mqtt_url);
    return client;
}

static void
__attribute__((noreturn)) _delete_task()
{
    ESP_LOGI(TAG, "Exiting task ..");
    (void)vTaskDelete(NULL);

    while (1) {  // FreeRTOS requires that tasks never return
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/*
 * Responsibilities:
 *   - Initially, check if there is a coredump in flash memory, then publish it to MQTT.
 *   - Subscribe to control topics, for diagnostic commands.
 *   - Connect to MQTT broker.
 *   - Subscribing or publishing on MQTT topics on behalf of other processes
 */

void
mqtt_task(void * ipc_void)
{
    // register control topics `opnpool/ctrl` and `opnpool/ctrl/ID`, where `ID` is the device name.
    // these control topics allows other devices clients issue diagnistic commands.
    // once connected to the broker, we'll subscribe to the topics.

 	ipc_t * ipc = ipc_void;
    {
        char * topic;
        assert( asprintf(&topic, "%s/%s", CONFIG_OPNPOOL_MQTT_CTRL_TOPIC, ipc->dev.name) >= 0 );
        _add_subscriber(topic, true);
        // don't free(topic) stays in subscribers linked list
        assert( asprintf(&topic, "%s", CONFIG_OPNPOOL_MQTT_CTRL_TOPIC) >= 0 );
        _add_subscriber(topic, true);
        // don't free(topic) stays in subscribers linked list
    }

    // event group indicates that we're connected to the MQTT broker

	_mqttEventGrp = xEventGroupCreate();
    esp_mqtt_client_handle_t const client = _connect2broker(ipc);
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT not provisioned");
        _delete_task();
    }

    // if there is a coredump in flash memory, then publish it to MQTT

    _forwardCoredump(ipc, client);

    // read messages from the `ipc->to_mqtt_q` so allow processes to subscribe or publish on topics.

	while (1) {
        ipc_to_mqtt_msg_t msg;
		if (xQueueReceive(ipc->to_mqtt_q, &msg, (TickType_t)(1000L / portTICK_PERIOD_MS)) == pdPASS) {

            switch (msg.dataType) {

                    // subscribe to a topic on behalf of `hass_task`
                case IPC_TO_MQTT_TYP_SUBSCRIBE: {  
                    _add_subscriber(msg.data, false);  // used when we have to reconnect to broker
                    esp_mqtt_client_subscribe(client, msg.data, 1);
                    if (CONFIG_OPNPOOL_DBGLVL_MQTTTASK > 1) {
                        ESP_LOGI(TAG, "Temp subscribed to \"%s\"", msg.data);
                    }
                    // don't free(msg.data), stays in _subscribers linked list
                    break;
                }

                    // publish using topic and message from `msg.data`
                case IPC_TO_MQTT_TYP_PUBLISH: {  // publish a message (from `hass_task`)
                    char const * const topic = msg.data;
                    char * message = strchr(msg.data, '\t');
                    *message++ = '\0';
                    if (CONFIG_OPNPOOL_DBGLVL_MQTTTASK > 1) {
                        ESP_LOGI(TAG, "tx %s: %s", topic, message);
                    }
                    esp_mqtt_client_publish(client, topic, message, strlen(message), 1, 0);
                    free(msg.data);
                    break;
                }

                    // publish to `/opnpool/data/SUB` where `SUB` is `msg.dataType` and the value is `msg.data`
                case IPC_TO_MQTT_TYP_PUBLISH_DATA_RESTART:  // publish response when restarting device (from `_dispatch_restart`)
                case IPC_TO_MQTT_TYP_PUBLISH_DATA_WHO:      // publish response with info about device (from `_dispatch_who`)
                case IPC_TO_MQTT_TYP_PUBLISH_DATA_DBG: {    // publish debug info (from e.g. `poolstate_rx`)
                    char * topic;
                    char const * const subtopic = ipc_to_mqtt_typ_str(msg.dataType);
                    if (subtopic) {
                        assert( asprintf(&topic, "%s/%s/%s", CONFIG_OPNPOOL_MQTT_DATA_TOPIC, subtopic, ipc->dev.name) >= 0);
                    } else {
                        assert( asprintf(&topic, "%s/%s", CONFIG_OPNPOOL_MQTT_DATA_TOPIC, ipc->dev.name) >= 0);
                    }
                    esp_mqtt_client_publish(client, topic, msg.data, strlen(msg.data), 1, 0);
                    free(topic);
                    free(msg.data);
                    break;
                }
            }
		}
	}
}