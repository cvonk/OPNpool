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

    datalink_pkt_t rx_pkt;
    network_msg_t rx_msg;
    bool txOpportunity;
    poolstate_t state;
    size_t json_size = 1024;
    char json[json_size];

    while (1) {
        if (datalink_rx_pkt(rs485_handle, &rx_pkt)) {
            //ESP_LOGI(TAG, "received datalink pkt");

            if (network_rx_msg(&rx_pkt, &rx_msg, &txOpportunity)) {
                //ESP_LOGI(TAG, "received network msg");

                if (poolstate_rx_update(&rx_msg, &state, ipc)) {
                    //ESP_LOGI(TAG, "poolstate updated");

                    poolstate_to_json(&state, json, json_size);
                    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
                }
            }
            if (txOpportunity) {

                network_msg_ctrl_circuit_set_t circuit_set = {
                        .circuit = 1 +1,
                        .value = 1,
                };
                network_msg_t tx_msg = {
                    .typ = MSG_TYP_CTRL_CIRCUIT_SET,
                    .u.ctrl_circuit_set = &circuit_set,
                };
                datalink_pkt_t tx_pkt;
                if (network_tx_msg(&tx_msg, &tx_pkt)) {
                    datalink_tx_pkt(rs485_handle, &tx_pkt);
                }

                skb_handle_t const txb = rs485_handle->dequeue(rs485_handle);
                if (txb) {
                    ESP_LOGW(TAG, "TX should happen here");

// 2BD do the transmit
                    size_t const dbg_size = 128;
                    char dbg[dbg_size];
                    (void) skb_print(TAG, txb, dbg, dbg_size);
                    ESP_LOGI(TAG, "tx{ %s}", dbg);

                    if (network_rx_msg(&tx_pkt, &rx_msg, &txOpportunity)) {
                        ESP_LOGI(TAG, "FEEDBACK received network msg");
                        if (poolstate_rx_update(&rx_msg, &state, ipc)) {
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