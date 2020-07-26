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
#include "../network/network_hdr.h"
#include "tx_buf/tx_buf.h"
#include "datalink.h"

static char const * const TAG = "datalink_tx";

static void
_enter_ic_head(datalink_ic_tx_head_t * const head, tx_buf_handle_t const txb, datalink_ctrl_typ_t const typ)
{
    head->ff = 0xFF;
    for (uint_least8_t ii = 0; ii < sizeof(datalink_preamble_ic); ii++) {
        head->preamble[ii] = datalink_preamble_ic[ii];
    }
    head->hdr.dst = datalink_devaddr(DATALINK_ADDRGROUP_CTRL, 0);
    head->hdr.typ = typ;
}

static void
_enter_ic_tail(datalink_ic_tx_tail_t * const tail, uint8_t const * const start, uint8_t const * const stop)
{
    uint8_t crcVal = 0;
    for (uint8_t const * byte = start; byte < stop; byte++) {
        crcVal += *byte;
    }
    tail->crc = crcVal;
}

static void
_enter_a5_head(datalink_a5_tx_head_t * const head, tx_buf_handle_t const txb, datalink_ctrl_typ_t const typ, size_t const data_len)
{
    head->ff = 0xFF;
    for (uint_least8_t ii = 0; ii < sizeof(datalink_preamble_a5); ii++) {
        head->preamble[ii] = datalink_preamble_a5[ii];
    }
    head->hdr.ver = 0x01;
    head->hdr.dst = datalink_devaddr(DATALINK_ADDRGROUP_CTRL, 0);
    head->hdr.src = datalink_devaddr(DATALINK_ADDRGROUP_REMOTE, 0);
    head->hdr.typ = typ;
    head->hdr.len = data_len;
}

static void
_enter_a5_tail(datalink_a5_tx_tail_t * const tail, uint8_t const * const start, uint8_t const * const stop)
{
    uint16_t crcVal = 0;
    for (uint8_t const * byte = start; byte < stop; byte++) {
        crcVal += *byte;
    }
    tail->crc[0] = crcVal >> 8;
    tail->crc[1] = crcVal & 0xFF;
}

void
datalink_tx_pkt(rs485_handle_t const rs485_handle, tx_buf_handle_t const txb, datalink_prot_t const prot, datalink_ctrl_typ_t const typ)
{
    size_t const data_len = txb->len;

    switch (prot) {
        case DATALINK_PROT_IC: {

            datalink_ic_tx_head_t * const head = (datalink_ic_tx_head_t *) tx_buf_push(txb, sizeof(datalink_ic_tx_head_t));
            _enter_ic_head(head, txb, typ);

            uint8_t * crc_start = head->preamble;
            uint8_t * crc_stop = txb->priv.tail;
            datalink_ic_tx_tail_t * const tail = (datalink_ic_tx_tail_t *) tx_buf_put(txb, sizeof(datalink_ic_tx_tail_t));
            _enter_ic_tail(tail, crc_start, crc_stop);
            break;
        }
        case DATALINK_PROT_A5_CTRL:
        case DATALINK_PROT_A5_PUMP: {

            datalink_a5_tx_head_t * const head = (datalink_a5_tx_head_t *) tx_buf_push(txb, sizeof(datalink_a5_tx_head_t));
            _enter_a5_head(head, txb, typ, data_len);

            uint8_t * crc_start = &head->preamble[sizeof(datalink_a5_preamble_t) - 1];
            uint8_t * crc_stop = txb->priv.tail;
            datalink_a5_tx_tail_t * const tail = (datalink_a5_tx_tail_t *) tx_buf_put(txb, sizeof(datalink_a5_tx_tail_t));
            _enter_a5_tail(tail, crc_start, crc_stop);
            break;
        }
    }
    size_t const dbg_size = 128;
    char dbg[dbg_size];
    (void) tx_buf_print(TAG, txb, dbg, dbg_size);
    ESP_LOGI(TAG, "%s: { %s}", datalink_prot_str(prot), dbg);

    rs485_handle->queue(rs485_handle, txb);
}
