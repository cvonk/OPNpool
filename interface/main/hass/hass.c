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
#include <time.h>

#include "../skb/skb.h"
#include "../rs485/rs485.h"
#include "../datalink/datalink.h"
#include "../datalink/datalink_pkt.h"
#include "../network/network.h"
#include "../poolstate/poolstate.h"

#include "../ipc/ipc.h"
#include "../utils/utils.h"
#include "hass.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "hass";

typedef esp_err_t (* dispatch_fnc_t)(char const * const dev_name, char const * const value_str, datalink_pkt_t * const pkt);

typedef struct dispatch_t {
    char const * const  dev_type;
    char const * const  dev_name;
    dispatch_fnc_t      fnc;
} dispatch_t;


static uint
_parse_topic(char * data, char * args[], uint const args_len) {

    uint ii = 0;
    char const * const delim = "/";
    char * save;
    char * p = strtok_r(data, delim, &save);
    while (p && ii < args_len) {
        args[ii++] = p;
        p = strtok_r(NULL, delim, &save);
    }
    return ii;
}

static esp_err_t
_dispatch_circuit_set(char const * const dev_name, char const * const value_str, datalink_pkt_t * const pkt)
{
    network_circuit_t circuit;
    if ((circuit = network_circuit_nr(dev_name)) != -1) {
        uint8_t const value = strcmp(value_str, "ON") == 0;

        network_msg_ctrl_circuit_set_t circuit_set = {
                .circuit = circuit +1,
                .value = value,
        };
        network_msg_t msg = {
            .typ = MSG_TYP_CTRL_CIRCUIT_SET,
            .u.ctrl_circuit_set = &circuit_set,
        };
        if (network_tx_msg(&msg, pkt)) {
            ESP_LOGW(TAG, "%s pkt=%p", __func__, pkt);
            return ESP_OK;
        } else {
            free(pkt);
        }
    }
    ESP_LOGE(TAG, "circuit err (%s)", dev_name);
    return ESP_FAIL;
}

static dispatch_t _dispatches[] = {
    { "switch", "aux1", _dispatch_circuit_set},
};

esp_err_t
hass_rx_mqtt(char * const topic, char const * const value_str, datalink_pkt_t * const pkt)
{
    char * args[5];
    uint8_t argc = _parse_topic(topic, args, ARRAY_SIZE(args));

    if (argc >= 5) {
        char const * const dev_type = args[1];
        char const * const dev_name = args[3];

        dispatch_t const * dispatch = _dispatches;
        for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
            if (strcmp(dev_type, dispatch->dev_type) == 0) {
                return dispatch->fnc(dev_name, value_str, pkt);
            }
        }
    }
    return ESP_FAIL;
}

void
hass_init(ipc_t const * const ipc)
{
    dispatch_t const * dispatch = _dispatches;
    for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
        char * base;
        asprintf(&base, "homeassistant/%s/%s/%s", dispatch->dev_type, ipc->dev.name, dispatch->dev_name);
        assert(base);

        char * set_topic, * state_topic;
        asprintf(&set_topic, "%s/%s", base, "set");
        asprintf(&state_topic, "%s/%s", base, "state");
        assert(set_topic && state_topic);

        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_SUBSCRIBE, set_topic, ipc);
        //ipc_send_to_mqtt(IPC_TO_MQTT_TYP_SUBSCRIBE, state_topic, ipc);
        free(set_topic);
        free(state_topic);

        char * cfg;
        asprintf(&cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                 "\"~\":\"%s\","
                 "\"name\":\"pool %s\","
                 "\"cmd_t\":\"~/set\","
                 "\"stat_t\":\"~/state\"}", base, base, dispatch->dev_name);
        assert(cfg);
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_ANNOUNCE, cfg, ipc);
        free(cfg);
        free(base);
    }
}

esp_err_t
hass_tx_state(poolstate_t const * const state, ipc_t const * const ipc)
{
    size_t json_size = 1024;
    char json[json_size];

    poolstate_to_json(state, json, json_size);
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
    return ESP_OK;
}