#pragma once

#include <esp_system.h>

typedef struct skb_priv_t {
    uint8_t * head;  // start of the pkt
    uint8_t * data;  // start of the payload
    uint8_t * tail;  // end of the payload
    uint8_t * end;   // end of the packet
} skb_priv_t;

typedef struct skb_t {
    skb_priv_t priv;
    size_t        len;   // amount of data in buf
    size_t        size;  // amount of mem alloc'ed
    uint8_t       buf[];
} skb_t;

typedef skb_t * skb_handle_t;

skb_handle_t skb_alloc(size_t size);
void skb_reserve(skb_handle_t const skb, size_t const header_len);
uint8_t * skb_put(skb_handle_t const skb, size_t const user_data_len);
uint8_t * skb_push(skb_handle_t const skb, size_t const header_len);
uint8_t * skb_pull(skb_handle_t const skb, size_t const header_len);
void skb_reset(skb_handle_t skb);
void skb_free(skb_handle_t const skb);
size_t skb_print(char const * const tag, skb_handle_t const skb, char * const buf, size_t const buf_size);
