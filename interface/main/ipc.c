/**
* @brief packet_task, packetizes RS485 byte stream from Pentair bus
 *
 * The Pentair controller uses two different protocols to communicate with its peripherals:
 *   - 	A5 has messages such as 0x00 0xFF <ldb> <sub> <dst> <src> <cfi> <len> [<data>] <chH> <ckL>
 *   -  IC has messages such as 0x10 0x02 <data0> <data1> <data2> .. <dataN> <ch> 0x10 0x03
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

#include "ipc.h"

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
ipc_to_mqtt_typ_str(network_heat_src_t const typ)
{
    return ELEM_AT(_ipc_to_mqtt_typs, typ, hex8_str(typ));
}

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
