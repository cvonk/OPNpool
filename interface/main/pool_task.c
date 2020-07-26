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
#include "network/network.h"
#include "poolstate/poolstate.h"
#include "ipc/ipc.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "pool_task";

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
    rs485_handle_t rs485_handle = rs485_init(&rs485_config);

    datalink_pkt_t datalink_pkt;
    datalink_pkt.skb = skb_alloc(DATALINK_MAX_HEAD_SIZE + DATALINK_MAX_DATA_SIZE + DATALINK_MAX_TAIL_SIZE);
    network_msg_t network_msg;
    bool txOpportunity;
    poolstate_t state;
    size_t json_size = 1024;
    char json[json_size];

    while (1) {
        if (datalink_rx_pkt(rs485_handle, &datalink_pkt)) {
            //ESP_LOGI(TAG, "received datalink pkt");

            if (network_rx_msg(&datalink_pkt, &network_msg, &txOpportunity)) {
                //ESP_LOGI(TAG, "received network msg");

                if (poolstate_rx_update(&network_msg, &state, ipc)) {
                    //ESP_LOGI(TAG, "poolstate updated");

                    poolstate_to_json(&state, json, json_size);
                    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
                }
            }
            if (txOpportunity) {

#if 0
                network_tx_circuit_set_msg(rs485_handle, 1, 1);
#endif
                skb_handle_t const txb = rs485_handle->dequeue(rs485_handle);
                if (txb) {
                    ESP_LOGW(TAG, "TX should happen here");
                    size_t const dbg_size = 128;
                    char dbg[dbg_size];
                    (void) skb_print(TAG, txb, dbg, dbg_size);
                    ESP_LOGI(TAG, "tx{ %s}", dbg);

                    datalink_pkt_t pkt;
                    pkt.prot = DATALINK_PROT_A5_CTRL;
                    pkt.head = (datalink_head_t *) txb->priv.data;
                    pkt.tail = (datalink_tail_t *) txb->priv.tail - sizeof(datalink_tail_a5_t);
                    pkt.data = txb->priv.data + sizeof(datalink_head_a5_t);
                    pkt.data_len = txb->len - sizeof(datalink_head_a5_t) - sizeof(datalink_tail_a5_t);
                    pkt.skb = txb;

                    if (network_rx_msg(&pkt, &network_msg, &txOpportunity)) {
                        ESP_LOGI(TAG, "FEEDBACK received network msg");

                        if (poolstate_rx_update(&network_msg, &state, ipc)) {
                            ESP_LOGI(TAG, "FEEDBACK poolstate updated");

                            poolstate_to_json(&state, json, json_size);
                            ESP_LOGI(TAG, "FEEDBACK %s", json);
                            ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
                        }
                    }
                    free(txb);
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}