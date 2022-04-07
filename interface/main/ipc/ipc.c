/**
 * @brief OPNpool - Inter Process Communication: mailbox messages exchanged between the tasks
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
        if (CONFIG_OPNPOOL_DBGLVL_IPC > 1) {
            ESP_LOGW(TAG, "to_mqtt_q full");
        }
        free(msg.data);
    }
    vTaskDelay(1);  // give `mqtt_task` a chance to catch up
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
        if (CONFIG_OPNPOOL_DBGLVL_IPC > 1) {
            ESP_LOGW(TAG, "to_pool_q full");
        }
        free(msg.data);
    }
}
