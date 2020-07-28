/**
 * @brief Data Link layer: bytes from the RS485 transceiver from data packets
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "../rs485/rs485.h"
#include "skb/skb.h"
#include "datalink.h"

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
    head->hdr.src = datalink_devaddr(DATALINK_ADDRGROUP_REMOTE, 0);
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

void
datalink_tx_pkt(rs485_handle_t const rs485_handle, datalink_pkt_t * const pkt)
{
    skb_handle_t const skb = pkt->skb;

    switch (pkt->prot) {
        case DATALINK_PROT_IC: {
            pkt->head = (datalink_head_t *) skb_push(skb, sizeof(datalink_head_ic_t));
            _enter_ic_head(&pkt->head->ic, skb, pkt->prot_typ);

            uint8_t * crc_start = pkt->head->ic.preamble;
            uint8_t * crc_stop = skb->priv.tail;
            datalink_tail_ic_t * const tail = (datalink_tail_ic_t *) skb_put(skb, sizeof(datalink_tail_ic_t));
            _enter_ic_tail(tail, crc_start, crc_stop);
            break;
        }
        case DATALINK_PROT_A5_CTRL:
        case DATALINK_PROT_A5_PUMP: {
            pkt->head = (datalink_head_t *) skb_push(skb, sizeof(datalink_head_a5_t));
            _enter_a5_head(&pkt->head->a5, skb, pkt->prot_typ, pkt->data_len);

            uint8_t * crc_start = &pkt->head->a5.preamble[sizeof(datalink_preamble_a5_t) - 1];
            uint8_t * crc_stop = skb->priv.tail;
            datalink_tail_a5_t * const tail = (datalink_tail_a5_t *) skb_put(skb, sizeof(datalink_tail_a5_t));
            _enter_a5_tail(tail, crc_start, crc_stop);
            break;
        }
    }
    if (CONFIG_POOL_DBG_DATALINK) {
        size_t const dbg_size = 128;
        char dbg[dbg_size];
        (void) skb_print(TAG, skb, dbg, dbg_size);
        ESP_LOGI(TAG, "%s: { %s}", datalink_prot_str(pkt->prot), dbg);
    }
    rs485_handle->queue(rs485_handle, skb);
}
