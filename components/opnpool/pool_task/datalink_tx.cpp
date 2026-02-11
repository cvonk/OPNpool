/**
 * @file datalink_tx.cpp
 * @brief Data Link layer: data packets to bytes for the RS485 transceiver
 *
 * @details
 * This file implements the data link layer transmitter for the OPNpool component,
 * responsible for constructing protocol-compliant packets for transmission over the RS485
 * bus. It provides functions to add protocol-specific headers and tails (including
 * preambles and checksums) for both A5 and IC protocols, ensuring correct framing and
 * integrity of outgoing messages. The implementation manages buffer manipulation,
 * protocol selection, and queues completed packets for transmission by the RS485 driver.
 * This layer enables reliable and standards-compliant communication with pool equipment
 * by encapsulating higher-level messages into properly formatted data link packets.
 *
 * ESPHome operates in a single-threaded environment, so explicit thread safety measures
 * are not required within the pool_task context.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2014, 2019, 2022, 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/log.h>

#include "rs485.h"
#include "skb.h"
#include "datalink.h"
#include "datalink_pkt.h"
#include "utils/enum_helpers.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

constexpr char TAG[] = "datalink_tx";

    // debug buffer size for logging
constexpr size_t DBG_SIZE = 128;

constexpr size_t DATALINK_PREAMBLE_IC_SIZE = sizeof(datalink_preamble_ic);
constexpr size_t DATALINK_PREAMBLE_A5_SIZE = sizeof(datalink_preamble_a5);

constexpr uint8_t A5_PROTOCOL_VERSION = 0x01;

/**
 * @brief          Fills the IC protocol packet header fields for transmission.
 *
 * @param[out] head Pointer to the IC protocol header structure to fill.
 * @param[in]  typ  Message type union for the packet.
 */
static void
_enter_ic_head(datalink_head_ic_t * const head, datalink_typ_t const typ)
{
    head->ff = 0xFF;
    for (uint_least8_t ii = 0; ii < DATALINK_PREAMBLE_IC_SIZE; ii++) {
        head->preamble[ii] = datalink_preamble_ic[ii];
    }

        // 2BD: we know the address of the controller from the broadcast.  use that.
    head->hdr.dst = datalink_addr_t::suntouch_controller();
    head->hdr.typ = typ.raw;
}

/**
 * @brief           Fills the IC protocol packet tail (checksum) for transmission.
 *
 * @param[out] tail  Pointer to the IC protocol tail structure to fill.
 * @param[in]  start Pointer to the start of the data for checksum calculation.
 * @param[in]  stop  Pointer to the end of the data for checksum calculation.
 */
static void
_enter_ic_tail(datalink_tail_ic_t * const tail, uint8_t const * const start, uint8_t const * const stop)
{
    tail->checksum[0] = (uint8_t) datalink_calc_checksum(start, stop);
}

/**
 * @brief              Fills the A5 protocol packet header fields for transmission.
 *
 * @param[out] head     Pointer to the A5 protocol header structure to fill.
 * @param[in]  typ      Message type union for the packet.
 * @param[in]  data_len Length of the data payload.
 */
static void
_enter_a5_head(datalink_head_a5_t * const head, datalink_addr_t const src, datalink_addr_t const dst, datalink_typ_t const typ, size_t const data_len)
{
    head->ff = 0xFF;
    for (uint_least8_t ii = 0; ii < DATALINK_PREAMBLE_A5_SIZE; ii++) {
        head->preamble[ii] = datalink_preamble_a5[ii];
    }
    head->hdr.ver = A5_PROTOCOL_VERSION;
    head->hdr.src = src;
    head->hdr.dst = dst;
    head->hdr.typ = typ.raw;
    head->hdr.len = data_len;
}

/**
 * @brief            Fills the A5 protocol packet tail (checksum) for transmission.
 *
 * @param[out] tail  Pointer to the A5 protocol tail structure to fill.
 * @param[in]  start Pointer to the start of the data for checksum calculation.
 * @param[in]  stop  Pointer to the end of the data for checksum calculation.
 */
static void
_enter_a5_tail(datalink_tail_a5_t * const tail, uint8_t const * const start, uint8_t const * const stop)
{
    uint16_t checksumVal = datalink_calc_checksum(start, stop);
    tail->checksum[0] = checksumVal >> 8;
    tail->checksum[1] = checksumVal & 0xFF;
}

/**
 * @brief Adds protocol headers and tails to a data packet and queues it for RS485 transmission.
 *
 * @details
 * This function constructs a protocol-compliant packet by adding the appropriate header
 * and tail (preamble and checksum) for the specified protocol (IC, A5/controller, or A5/pump).
 * It prepares the socket buffer for transmission, and enqueues the completed packet for
 * transmission by the RS485 driver. This function is typically called from the pool_task
 * to send messages to pool equipment.
 *
 * @param[in] rs485 Pointer to the RS485 interface handle.
 * @param[in] pkt   Pointer to the datalink packet structure to be transmitted.
 */
void
datalink_tx_pkt_queue(rs485_handle_t const rs485, datalink_pkt_t const * const pkt)
{
    skb_handle_t const skb = pkt->skb;

    switch (pkt->prot) {
        case datalink_prot_t::IC: {
            datalink_head_ic_t * const head = (datalink_head_ic_t *) skb_push(skb, sizeof(datalink_head_ic_t));
            _enter_ic_head(head, pkt->typ);

            uint8_t * checksum_start = head->preamble;
            uint8_t * checksum_stop = skb->priv.tail;
            datalink_tail_ic_t * const tail = (datalink_tail_ic_t *) skb_put(skb, sizeof(datalink_tail_ic_t));
            _enter_ic_tail(tail, checksum_start, checksum_stop);
            break;
        }
        case datalink_prot_t::A5_CTRL:
        case datalink_prot_t::A5_PUMP: {
            datalink_head_a5_t * const head = (datalink_head_a5_t *) skb_push(skb, sizeof(datalink_head_a5_t));
            _enter_a5_head(head, pkt->src, pkt->dst, pkt->typ, pkt->data_len);

            uint8_t * checksum_start = head->preamble + DATALINK_PREAMBLE_A5_SIZE - 1;
            uint8_t * checksum_stop = skb->priv.tail;
            datalink_tail_a5_t * const tail = (datalink_tail_a5_t *) skb_put(skb, sizeof(datalink_tail_a5_t));
            _enter_a5_tail(tail, checksum_start, checksum_stop);
            break;
        }
        default: {
            ESP_LOGE(TAG, "Unsupported protocol type: %02X", static_cast<uint8_t>(pkt->prot));
            return;
        }
    }
    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        size_t const dbg_size = DBG_SIZE;
        char dbg[dbg_size];
        (void) skb_print(skb, dbg, dbg_size);
        ESP_LOGV(TAG, " %s: { %s}", enum_str(pkt->prot), dbg);
    }

        // queue for transmission by `pool_task`
    rs485->queue(rs485, pkt);
}

} // namespace opnpool
} // namespace esphome