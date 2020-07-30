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
#include "hass/hass.c"
#include "ipc/ipc.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

//static char const * const TAG = "pool_task";

static bool
_service_pkts_from_rs485(rs485_handle_t const rs485, ipc_t const * const ipc)
{
    poolstate_t state;
    bool txOpportunity = false;
    datalink_pkt_t pkt;
    network_msg_t msg;

    if (datalink_rx_pkt(rs485, &pkt) == ESP_OK) {
        if (network_rx_msg(&pkt, &msg, &txOpportunity) == ESP_OK) {
            if (poolstate_rx_update(&msg, &state, ipc) == ESP_OK) {
                hass_tx_state(&state, ipc);
            }
        }
        free(pkt.skb);
    }
    return txOpportunity;
}

static void
_service_requests_from_mqtt(rs485_handle_t rs485, ipc_t const * const ipc)
{
    ipc_to_pool_msg_t queued_msg;

    // service SET requests received over MQTT
    if (xQueueReceive(ipc->to_pool_q, &queued_msg, (TickType_t)0) == pdPASS) {
        assert(queued_msg.dataType == IPC_TO_POOL_TYP_SET);
        datalink_pkt_t * const pkt = malloc(sizeof(datalink_pkt_t));
        if (hass_rx_mqtt(queued_msg.topic, queued_msg.data, pkt) == ESP_OK) {
            datalink_tx_pkt_queue(rs485, pkt);  // pkt and pkt->skb freed by mailbox recipient
        }
        free(queued_msg.data);
    }
}

static void
_forward_queued_pkt_to_rs485(rs485_handle_t const rs485, ipc_t const * const ipc)
{
    datalink_pkt_t const * const pkt = rs485->dequeue(rs485);
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

        // pretent that we received our own message
        poolstate_t state;
        bool txOpportunity = false;
        network_msg_t msg;
        if (network_rx_msg(pkt, &msg, &txOpportunity) == ESP_OK) {
            if (poolstate_rx_update(&msg, &state, ipc) == ESP_OK) {
                hass_tx_state(&state, ipc);
            }
        }
        free(pkt->skb);
        free(pkt);
    }
}

void
pool_task(void * ipc_void)
{
 	ipc_t * const ipc = ipc_void;
    rs485_handle_t const rs485 = rs485_init();

    hass_init(ipc);

    while (1) {

        _service_requests_from_mqtt(rs485, ipc);

        if (_service_pkts_from_rs485(rs485, ipc)) {

            _forward_queued_pkt_to_rs485(rs485, ipc);
        }
    }
}