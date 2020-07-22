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

#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "rs485/rs485.h"
#include "datalink/datalink.h"
#include "network/network.h"
#include "poolstate/poolstate.h"
#include "ipc_msgs.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "pool_task";

static bool
_good_time_to_tx(datalink_pkt_t * msg)
{
    return (msg->proto == NETWORK_PROT_A5) &&
        (network_group_addr(msg->hdr.src) == NETWORK_ADDRGROUP_CTRL) &&
        (network_group_addr(msg->hdr.dst) == NETWORK_ADDRGROUP_ALL);
}

void
pool_task(void * ipc_void)
{
 	//ipc_t * const ipc = ipc_void;

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

    datalink_pkt_t datalink_pkt; // poolstate_t state;
    network_msg_t network_msg;
    poolstate_t poolstate;

    while (1) {
        if (datalink_receive_pkt(rs485_handle, &datalink_pkt)) {
            ESP_LOGI(TAG, "received datalink pkt");

            if (network_receive_msg(&datalink_pkt, &network_msg)) {
                ESP_LOGI(TAG, "received network msg");

                if (poolstate_receive_update(&network_msg, &poolstate)) {

                }

                //interpetate and update the poolstate accordingly
            }

            if (_good_time_to_tx(&datalink_pkt)) {
                // read incoming mailbox for things to transmit
                //   and transmit them
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}