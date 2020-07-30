/**
* @brief Inter Process Communication: mailbox messages exchanged between the tasks
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "utils/utils.h"
#include "ipc/ipc.h"
#include "skb/skb.h"

static char const * const TAG = "ipc";

/**
 * network_heat_src_t
 **/

static const char * const _ipc_to_mqtt_typs[] = {
#define XX(num, name) #name,
  IPC_TO_MQTT_TYP_MAP(XX)
#undef XX
};

const char *
ipc_to_mqtt_typ_str(ipc_to_mqtt_typ_t const typ)
{
    return ELEM_AT(_ipc_to_mqtt_typs, typ, hex8_str(typ));
}

/**
 * ipc_send_to_mqtt
 **/

void
ipc_send_to_mqtt(ipc_to_mqtt_typ_t const dataType, char const * const data, ipc_t const * const ipc)
{
    ipc_to_mqtt_msg_t msg = {
        .dataType = dataType,
        .data = strdup(data)
    };
    assert(msg.data);
    if (xQueueSendToBack(ipc->to_mqtt_q, &msg, 0) != pdPASS) {
        ESP_LOGE(TAG, "to_mqtt_q full");
        free(msg.data);
    }
}

/**
 * ipc_send_to_pool
 **/

void
ipc_send_to_pool(ipc_to_pool_typ_t const dataType, char const * const topic, size_t const topic_len, char const * const data, size_t const data_len, ipc_t const * const ipc)
{
    ipc_to_pool_msg_t msg = {
        .dataType = dataType,
        .topic = strndup(topic, topic_len),
        .data = strndup(data, data_len),
    };
    assert(msg.data);
    if (xQueueSendToBack(ipc->to_pool_q, &msg, 0) != pdPASS) {
        ESP_LOGE(TAG, "to_pool_q full");
        free(msg.data);
    }
}
