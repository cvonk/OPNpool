/**
 * @brief Socket buffers: socket buf inspired continuous memory for tx
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>

#include "skb.h"

//static char const * const TAG = "skb";

skb_handle_t
skb_alloc(size_t size)
{
    skb_t * const skb = malloc(sizeof(skb_t) + size);
    assert(skb);
    skb->len = 0;
    skb->size = size;
    skb->priv.head =
    skb->priv.data =
    skb->priv.tail = skb->buf;
    skb->priv.end  = skb->buf + size;
    return skb;
}

void
skb_free(skb_handle_t const skb)
{
    free(skb);
}

// reserve headroom for protocol headers
void
skb_reserve(skb_handle_t const skb, size_t const header_len)
{
    assert(skb->priv.head == skb->priv.tail);  // only once after skb_alloc()
    assert(skb->priv.tail + header_len <= skb->priv.end);
    skb->priv.data += header_len;
    skb->priv.tail += header_len;
}

// returns pointer to write user data
uint8_t *
skb_put(skb_handle_t const skb, size_t const user_data_len)
{
    assert(skb->priv.tail + user_data_len <= skb->priv.end);
    uint8_t * const ret = skb->priv.tail;
    skb->len += user_data_len;
    skb->priv.tail += user_data_len;
    return ret;
}

// returns pointer to write header protocol data
uint8_t *
skb_push(skb_handle_t const skb, size_t const header_len)
{
    assert(skb->priv.data - header_len >= skb->priv.head);
    skb->len += header_len;
    return skb->priv.data -= header_len;
}

// reclaims header, and returns pointer to user data
uint8_t *
skb_pull(skb_handle_t const skb, size_t const header_len)
{
    assert(skb->priv.data - header_len >= skb->priv.head);
    skb->len -= header_len;
    return skb->priv.data -= header_len;
}

void
skb_reset(skb_handle_t skb)
{
    skb->len  = 0;
    skb->priv.head =
    skb->priv.data =
    skb->priv.tail = skb->buf;
    skb->priv.end  = skb->buf + skb->size;
}

size_t
skb_print(char const * const tag, skb_handle_t const skb, char * const buf, size_t const buf_size)
{
    size_t len = 0;
    for (size_t ii = 0; ii < skb->len; ii++) {
        len += snprintf(buf + len, buf_size - len, "%02x ", skb->priv.data[ii]);
    }
    return len;
}
