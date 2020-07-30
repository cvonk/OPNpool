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

#include "skb/skb.h"
#include "rs485/rs485.h"
#include "datalink/datalink.h"
#include "datalink/datalink_pkt.h"
#include "network/network.h"
#include "poolstate/poolstate.h"
#include "ipc/ipc.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "pool_task";

#if 0
static uint
_parse_args(char * data, char * args[], uint const args_len) {

    uint ii = 0;
    char const * const delim = " ";
    char * save;
    char * p = strtok_r(data, delim, &save);
    while (p && ii < args_len) {
        args[ii++] = p;
        p = strtok_r(NULL, delim, &save);
    }
    return ii;
}
#endif
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

static datalink_pkt_t *
_dispatch_circuit_set(char const * const dev_name, char const * const value_str)
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
        datalink_pkt_t * const pkt = malloc(sizeof(datalink_pkt_t));
        if (network_tx_msg(&msg, pkt)) {
            ESP_LOGW(TAG, "%s pkt=%p", __func__, pkt);
            return pkt;
        } else {
            free(pkt);
        }
    }
    ESP_LOGE(TAG, "circuit err (%s)", dev_name);
    return NULL;
}

typedef datalink_pkt_t * (* dispatch_fnc_t)(char const * const dev_name, char const * const value_str);

typedef struct dispatch_t {
    char const * const  dev_type;
    char const * const  dev_name;
    dispatch_fnc_t      fnc;
} dispatch_t;

static dispatch_t _dispatches[] = {
    { "switch", "aux1", _dispatch_circuit_set},
};

static datalink_pkt_t *
_dispatch(char * const topic, char const * const value_str)
{
    char * args[5];
    uint8_t argc = _parse_topic(topic, args, ARRAY_SIZE(args));
    if (argc >= 5) {
        char const * const dev_type = args[1];
        char const * const dev_name = args[3];

        dispatch_t const * dispatch = _dispatches;
        for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
            if (strcmp(dev_type, dispatch->dev_type) == 0) {
                return dispatch->fnc(dev_name, value_str);
            }
        }
    }
    return NULL;
}

static void
_announce_mqtt(ipc_t const * const ipc)
{

    // homeassistant/switch/esp32-wrover-1/AUX1/config homeassistant/switch/esp32-wrover-1/AUX1/config

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

void
pool_task(void * ipc_void)
{
 	ipc_t * const ipc = ipc_void;

    rs485_config_t rs485_config = {
        .rx_pin = CONFIG_POOL_RS485_RXPIN,
        .tx_pin = CONFIG_POOL_RS485_TXPIN,
        .rts_pin = CONFIG_POOL_RS485_RTSPIN,
        .uart_port = 2,
        .uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
            .use_ref_tick = false,
        }
    };
    rs485_handle_t const rs485 = rs485_init(&rs485_config);
    rs485->tx_mode(false);

    _announce_mqtt(ipc);

    bool txOpportunity;
    poolstate_t state;
    size_t json_size = 1024;
    char json[json_size];
    ipc_to_pool_msg_t queued_msg;

    while (1) {
		if (xQueueReceive(ipc->to_pool_q, &queued_msg, (TickType_t)0) == pdPASS) {

            assert(queued_msg.dataType == IPC_TO_POOL_TYP_REQ);
            ESP_LOGI(TAG, "received mqtt \"%s\": \"%s\"", queued_msg.topic, queued_msg.data);

            datalink_pkt_t const * const pkt = _dispatch(queued_msg.topic, queued_msg.data);
            if (pkt) {
                datalink_tx_queue_pkt(rs485, pkt);  // pkt and pkt->skb freed by mailbox recipient
            }
            char * payload;
            assert( asprintf(&payload, "{\"response\":{\"status\":\"%s\",\"req\":\"%s\" } }", pkt ? "OK" : "error", queued_msg.data) );
            ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, payload, ipc);
            free(payload);
            free(queued_msg.data);
        }
        {
            datalink_pkt_t pkt;
            network_msg_t msg;
            if (datalink_rx_pkt(rs485, &pkt)) {
                if (network_rx_msg(&pkt, &msg, &txOpportunity)) {
                    if (poolstate_rx_update(&msg, &state, ipc)) {
                        poolstate_to_json(&state, json, json_size);
                        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
                    }
                }
                free(pkt.skb);
            }
            if (txOpportunity) {
                if (0) {
                    network_msg_ctrl_circuit_set_t circuit_set = {
                            .circuit = 1 +1,
                            .value = 1,
                    };
                    network_msg_t msg = {
                        .typ = MSG_TYP_CTRL_CIRCUIT_SET,
                        .u.ctrl_circuit_set = &circuit_set,
                    };
                    datalink_pkt_t * const pkt = malloc(sizeof(datalink_pkt_t));
                    if (network_tx_msg(&msg, pkt)) {
                        datalink_tx_queue_pkt(rs485, pkt);
                    }
                }
                {
                    datalink_pkt_t * const pkt = rs485->dequeue(rs485);
                    if (pkt) {
                        if (CONFIG_POOL_DBG_POOLTASK) {
                            size_t const dbg_size = 128;
                            char dbg[dbg_size];
                            assert(pkt->skb);
                            (void) skb_print(TAG, pkt->skb, dbg, dbg_size);
                            ESP_LOGI(TAG, "tx { %s}", dbg);
                        }
                        rs485->tx_mode(true);
                        rs485->write_bytes(pkt->skb->priv.data, pkt->skb->len);
                        rs485->tx_mode(false);

                        // pretent we received our own message
                        network_msg_t msg;
                        if (network_rx_msg(pkt, &msg, &txOpportunity)) {
                            if (poolstate_rx_update(&msg, &state, ipc)) {
                                poolstate_to_json(&state, json, json_size);
                                ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
                            }
                        }
                        free(pkt->skb);
                        free(pkt);
                    }
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}