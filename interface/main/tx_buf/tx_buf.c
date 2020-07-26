/**
 * @brief Transmit buffers: socket buf inspired continuous memory for tx
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>

#include "tx_buf.h"

//static char const * const TAG = "tx_buf";

tx_buf_handle_t
alloc_txb(size_t size)
{
    tx_buf_t * const txb = malloc(sizeof(tx_buf_t) + size);
    assert(txb);
    txb->len  = 0;
    txb->priv.head =
    txb->priv.data =
    txb->priv.tail = txb->buf;
    txb->priv.end  = txb->buf + size;
    return txb;
}

void
free_txb(tx_buf_handle_t const txb)
{
    free(txb);
}

// reserve headroom for protocol headers
void
tx_buf_reserve(tx_buf_handle_t const txb, size_t const header_len)
{
    assert(txb->priv.head == txb->priv.tail);  // only once after tx_buf_alloc()
    assert(txb->priv.tail + header_len <= txb->priv.end);
    txb->priv.data += header_len;
    txb->priv.tail += header_len;
}

// returns pointer to write user data
uint8_t *
tx_buf_put(tx_buf_handle_t const txb, size_t const user_data_len)
{
    assert(txb->priv.tail + user_data_len <= txb->priv.end);
    uint8_t * const ret = txb->priv.tail;
    txb->len += user_data_len;
    txb->priv.tail += user_data_len;
    return ret;
}

// returns pointer to write header protocol data
uint8_t *
tx_buf_push(tx_buf_handle_t const txb, size_t const header_len)
{
    assert(txb->priv.data - header_len >= txb->priv.head);
    txb->len += header_len;
    return txb->priv.data -= header_len;
}

// reclaims header, and returns pointer to user data
uint8_t *
tx_buf_pull(tx_buf_handle_t const txb, size_t const header_len)
{
    assert(txb->priv.data - header_len >= txb->priv.head);
    txb->len -= header_len;
    return txb->priv.data -= header_len;
}

size_t
tx_buf_print(char const * const tag, tx_buf_handle_t const txb, char * const buf, size_t const buf_size)
{
    size_t len = 0;
    for (size_t ii = 0; ii < txb->len; ii++) {
        len += snprintf(buf + len, buf_size - len, "%02x ", txb->priv.data[ii]);
    }
    return len;
}
