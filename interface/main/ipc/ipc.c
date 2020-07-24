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
    toMqttMsg_t msg = {
        .dataType = dataType,
        .data = strdup(data)
    };
    assert(msg.data);
    if (xQueueSendToBack(ipc->to_mqtt_q, &msg, 0) != pdPASS) {
        ESP_LOGE(TAG, "to_mqtt_q full");
        free(msg.data);
    }
}
