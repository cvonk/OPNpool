/**
* @brief mqtt_client_task, fowards scan results to and control messages from MQTT broker
 **/
// Copyright © 2020, Coert Vonk
// SPDX-License-Identifier: MIT

#include <sdkconfig.h>
#include <stdlib.h>
#include <string.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
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
#include "ipc_msgs.h"
#include "coredump_to_server.h"

#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))
#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

static char const * const TAG = "mqtt_task";

static EventGroupHandle_t _mqttEventGrp = NULL;
typedef enum {
	MQTT_EVENT_CONNECTED_BIT = BIT0
} mqttEvent_t;

static struct {
    char * ctrl;
    char * ctrlGroup;
} _topic;

static esp_mqtt_client_handle_t _connect2broker(ipc_t const * const ipc);  // forward decl

void
sendToMqtt(toMqttMsgType_t const dataType, char const * const data, ipc_t const * const ipc)
{
    toMqttMsg_t msg = {
        .dataType = dataType,
        .data = strdup(data)
    };
    assert(msg.data);
    if (xQueueSendToBack(ipc->toMqttQ, &msg, 0) != pdPASS) {
        ESP_LOGE(TAG, "toMqttQ full");
        free(msg.data);
    }
}

static esp_err_t
_mqttEventHandler(esp_mqtt_event_handle_t event) {

    ipc_t * const ipc = event->user_context;

	switch (event->event_id) {
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Broker disconnected");
            xEventGroupClearBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT);
        	// reconnect is part of the SDK
            break;
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Broker connected");
            xEventGroupSetBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT);
            ipc->dev.connectCnt.mqtt++;
            esp_mqtt_client_subscribe(event->client, _topic.ctrl, 1);
            esp_mqtt_client_subscribe(event->client, _topic.ctrlGroup, 1);
            ESP_LOGI(TAG, " subscribed to \"%s\", \"%s\"", _topic.ctrl, _topic.ctrlGroup);
            break;
        case MQTT_EVENT_DATA:
            if (event->topic && event->data_len == event->total_data_len) {  // quietly ignores chunked messaegs

                if (strncmp("restart", event->data, event->data_len) == 0) {

                    sendToMqtt(TO_MQTT_MSGTYPE_RESTART, "{ \"response\": \"restarting\" }", ipc);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    esp_restart();

                } else if (strncmp("push", event->data, event->data_len) == 0) {

                    sendToMqtt(TO_MQTT_MSGTYPE_PUSH, "{ \"response\": \"pushed by MQTT\" }", ipc);
                    sendToClient(TO_CLIENT_MSGTYPE_TRIGGER, "mqtt push", ipc);

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
                        ipc->dev.connectCnt.wifi, ipc->dev.ipAddr, ap_info.ssid, ap_info.rssi,
                        ipc->dev.connectCnt.mqtt, heap_caps_get_free_size(MALLOC_CAP_8BIT)
                    );
                    assert(payload_len >= 0);
                    sendToMqtt(TO_MQTT_MSGTYPE_WHO, payload, ipc);
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
            .uri = CONFIG_CLOCK_MQTT_URL,
        };
        client = esp_mqtt_client_init(&mqtt_cfg);
        //ESP_ERROR_CHECK(esp_mqtt_client_set_uri(client, CONFIG_CLOCK_MQTT_URL));
        ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    }
	assert(xEventGroupWaitBits(_mqttEventGrp, MQTT_EVENT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY));
    return client;
}

static char const *
_type2subtopic(toMqttMsgType_t const type)
{
    struct mapping {
        toMqttMsgType_t const type;
        char const * const subtopic;
    } mapping[] = {
        { TO_MQTT_MSGTYPE_RESTART, "restart" },
        { TO_MQTT_MSGTYPE_PUSH, "push" },
        { TO_MQTT_MSGTYPE_WHO, "who" },
        { TO_MQTT_MSGTYPE_DBG, "dbg" },
    };
    for (uint ii = 0; ii < ARRAYSIZE(mapping); ii++) {
        if (type == mapping[ii].type) {
            return mapping[ii].subtopic;
        }
    }
    return NULL;
}

#if 0
static void
_getCoredump(ipc_t * ipc, esp_mqtt_client_handle_t const client)
{
    // BIN or ELF coredumping doesn't appear ready for primetime on SDK 4.1-beta2, try on 'master'
    esp_core_dump_init();
    size_t coredump_addr;
    size_t coredump_size;
    esp_err_t err = esp_core_dump_image_get(&coredump_addr, &coredump_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Coredump no image (%s)", esp_err_to_name(err));
        ESP_LOGW(TAG, " address %x > chip->size %x, ", coredump_addr, esp_flash_default_chip->size);
        ESP_LOGW(TAG, " address + length %x > chip->size %x", coredump_addr + coredump_size, esp_flash_default_chip->size);
        return;
    }
    size_t const chunk_len = 64;  //256
    size_t const str_len = 8 + 1 + chunk_len * 2 + 1;
    uint8_t * const chunk = malloc(chunk_len);
    char * const str = malloc(str_len);
    assert(chunk && str);

    ESP_LOGI(TAG, "Coredump is %u bytes", coredump_size);

    char * topic;
    asprintf(&topic, "%s/coredump/%s", CONFIG_CLOCK_MQTT_DATA_TOPIC, ipc->dev.name);

    for (size_t offset = 0; offset < coredump_size; offset += chunk_len) {

        uint const read_len = MIN(chunk_len, coredump_size - offset);
        //ESP_LOGI(TAG, "offset=0x%x coredump_size=%u, read_len=%u", offset, coredump_size, read_len);
        //if (esp_partition_read(part, offset, chunk, read_len) != ESP_OK) {
        if (esp_flash_read(esp_flash_default_chip, chunk, coredump_addr + offset , read_len)) {
            ESP_LOGE(TAG, "Coredump read failed");
            break;
        }
        uint len = 0;
        len += snprintf(str + len, str_len - len, "%08x ", offset);
        for (uint ii = 0; ii < read_len; ii++) {
            len += snprintf(str + len, str_len - len, "%02x", chunk[ii]);
        }
        esp_mqtt_client_publish(client, topic, str, len, 1, 0);
    }
    free(topic);
    free(chunk);
    free(str);

    //esp_flash_erase_region(esp_flash_default_chip, coredump_addr, coredump_size);
}
#endif

static esp_err_t
_coredump_to_server_begin_cb(void * priv)
{
    printf("================= CORE DUMP START =================\n");
    return ESP_OK;
}

static esp_err_t
_coredump_to_server_end_cb(void * priv)
{
    printf("================= CORE DUMP END ===================\n");
    return ESP_OK;
}

static esp_err_t
_coredump_to_server_write_cb(void * priv, char const * const str)
{
    printf("%s\r\n", str);
    return ESP_OK;
}

void
mqtt_task(void * ipc_void) {

    ESP_LOGI(TAG, "starting ..");
	ipc_t * ipc = ipc_void;

    _topic.ctrl      = malloc(strlen(CONFIG_CLOCK_MQTT_CTRL_TOPIC) + 1 + strlen(ipc->dev.name) + 1);
    _topic.ctrlGroup = malloc(strlen(CONFIG_CLOCK_MQTT_CTRL_TOPIC) + 1);
    assert(_topic.ctrl && _topic.ctrlGroup);
    sprintf(_topic.ctrl, "%s/%s", CONFIG_CLOCK_MQTT_CTRL_TOPIC, ipc->dev.name);
    sprintf(_topic.ctrlGroup, "%s", CONFIG_CLOCK_MQTT_CTRL_TOPIC);

	_mqttEventGrp = xEventGroupCreate();
    esp_mqtt_client_handle_t const client = _connect2broker(ipc);

    coredump_to_server_config_t coredump_cfg = {
        .start = _coredump_to_server_begin_cb,
        .end = _coredump_to_server_end_cb,
        .write = _coredump_to_server_write_cb,
        .priv = NULL,
    };
    coredump_to_server(&coredump_cfg);

	while (1) {
        toMqttMsg_t msg;
		if (xQueueReceive(ipc->toMqttQ, &msg, (TickType_t)(1000L / portTICK_PERIOD_MS)) == pdPASS) {

            char * topic;
            char const * const subtopic = _type2subtopic(msg.dataType);
            if (subtopic) {
                asprintf(&topic, "%s/%s/%s", CONFIG_CLOCK_MQTT_DATA_TOPIC, subtopic, ipc->dev.name);
            } else {
                asprintf(&topic, "%s/%s", CONFIG_CLOCK_MQTT_DATA_TOPIC, ipc->dev.name);
            }
            esp_mqtt_client_publish(client, topic, msg.data, strlen(msg.data), 1, 0);
            free(topic);
            free(msg.data);
		}
	}
}