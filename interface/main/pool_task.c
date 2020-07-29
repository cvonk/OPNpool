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

    bool txOpportunity;
    poolstate_t state;
    size_t json_size = 1024;
    char json[json_size];
    ipc_to_pool_msg_t queued_msg;

    while (1) {
		if (xQueueReceive(ipc->to_pool_q, &queued_msg, (TickType_t)0) == pdPASS) {

            assert(queued_msg.dataType == IPC_TO_POOL_TYP_REQ);
            ESP_LOGI(TAG, "received mqtt \"%s\"", queued_msg.data);
            char * args[3];
            uint8_t argc = _parse_args(queued_msg.data, args, ARRAY_SIZE(args));

            if (argc >= 3 && strcmp(args[0], "CTRL_CIRCUIT_SET") == 0 && strcmp(args[1], "AUX") == 0) {

                uint8_t const circuit = 1;  // args[1] == "AUX1"
                uint8_t const value = atoi(args[2]) ? 1 : 0;
                ESP_LOGI(TAG, "%s %u %u", args[0], circuit, value);

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
                    datalink_tx_queue_pkt(rs485, pkt);
                }
                char * payload;
                assert(asprintf(&payload, "{ \"response\": { \"%s\": {\"%u\", %u } }", args[0], circuit, value));
                ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, payload, ipc);
                free(payload);
                free(queued_msg.data);
            }
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
                            ESP_LOGW(TAG, "%p len=%u", pkt->skb->priv.data, pkt->skb->len);
                            size_t const dbg_size = 128;
                            char dbg[dbg_size];
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