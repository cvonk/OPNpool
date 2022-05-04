/**
 * @brief OPNpool - Data Link layer: bytes from the RS485 transceiver from data packets
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
 * SPDX-FileCopyrightText: Copyright 2014,2019,2022 Coert Vonk
 */

#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "../rs485/rs485.h"
#include "skb/skb.h"
#include "datalink.h"
#include "datalink_pkt.h"

static char const * const TAG = "datalink_tx";

static void
_enter_ic_head(datalink_head_ic_t * const head, skb_handle_t const txb, uint8_t const prot_typ)
{
    head->ff = 0xFF;
    for (uint_least8_t ii = 0; ii < sizeof(datalink_preamble_ic); ii++) {
        head->preamble[ii] = datalink_preamble_ic[ii];
    }
    head->hdr.dst = datalink_devaddr(DATALINK_ADDRGROUP_CTRL, 0);
    head->hdr.typ = prot_typ;
}

static void
_enter_ic_tail(datalink_tail_ic_t * const tail, uint8_t const * const start, uint8_t const * const stop)
{
    tail->crc[0] = (uint8_t) datalink_calc_crc(start, stop);
}

static void
_enter_a5_head(datalink_head_a5_t * const head, skb_handle_t const txb, uint8_t const prot_typ, size_t const data_len)
{
    head->ff = 0xFF;
    for (uint_least8_t ii = 0; ii < sizeof(datalink_preamble_a5); ii++) {
        head->preamble[ii] = datalink_preamble_a5[ii];
    }
    head->hdr.ver = 0x01;
    head->hdr.dst = datalink_devaddr(DATALINK_ADDRGROUP_CTRL, 0);
    head->hdr.src = datalink_devaddr(DATALINK_ADDRGROUP_REMOTE, 2);  // 2BD 0x20 is the wired remote; 0x22 is the wireless remote (Screen Logic, or any app)
    head->hdr.typ = prot_typ;
    head->hdr.len = data_len;
}

static void
_enter_a5_tail(datalink_tail_a5_t * const tail, uint8_t const * const start, uint8_t const * const stop)
{
    uint16_t crcVal = datalink_calc_crc(start, stop);
    tail->crc[0] = crcVal >> 8;
    tail->crc[1] = crcVal & 0xFF;
}

/*
 * Adds a header and tail to the packet and queues it for transmission on the RS-485 bus.
 * Called from `pool_task`
 */

void
datalink_tx_pkt_queue(rs485_handle_t const rs485, datalink_pkt_t const * const pkt)
{
    skb_handle_t const skb = pkt->skb;

    switch (pkt->prot) {
        case DATALINK_PROT_IC: {
            datalink_head_ic_t * const head = (datalink_head_ic_t *) skb_push(skb, sizeof(datalink_head_ic_t));
            _enter_ic_head(head, skb, pkt->prot_typ);

            uint8_t * crc_start = head->preamble;
            uint8_t * crc_stop = skb->priv.tail;
            datalink_tail_ic_t * const tail = (datalink_tail_ic_t *) skb_put(skb, sizeof(datalink_tail_ic_t));
            _enter_ic_tail(tail, crc_start, crc_stop);
            break;
        }
        case DATALINK_PROT_A5_CTRL:
        case DATALINK_PROT_A5_PUMP: {
            datalink_head_a5_t * const head = (datalink_head_a5_t *) skb_push(skb, sizeof(datalink_head_a5_t));
            _enter_a5_head(head, skb, pkt->prot_typ, pkt->data_len);

            uint8_t * crc_start = head->preamble + sizeof(datalink_preamble_a5_t) - 1;
            uint8_t * crc_stop = skb->priv.tail;
            datalink_tail_a5_t * const tail = (datalink_tail_a5_t *) skb_put(skb, sizeof(datalink_tail_a5_t));
            _enter_a5_tail(tail, crc_start, crc_stop);
            break;
        }
    }
    if (CONFIG_OPNPOOL_DBGLVL_DATALINK >1) {
        size_t const dbg_size = 128;
        char dbg[dbg_size];
        (void) skb_print(TAG, skb, dbg, dbg_size);
        ESP_LOGI(TAG, " %s: { %s}", datalink_prot_str(pkt->prot), dbg);
    }

    // queue for transmission by `pool_task`

    rs485->queue(rs485, pkt);
}
