#pragma once

#include <esp_system.h>

typedef struct tx_buf_priv_t {
    uint8_t * head;  // start of the pkt
    uint8_t * data;  // start of the payload
    uint8_t * tail;  // end of the payload
    uint8_t * end;   // end of the packet
} tx_buf_priv_t;

typedef struct tx_buf_t {
    tx_buf_priv_t priv;
    size_t        len;   // amount of data in buf
    uint8_t       buf[];
} tx_buf_t;

typedef tx_buf_t * tx_buf_handle_t;

tx_buf_handle_t alloc_txb(size_t size);
void tx_buf_reserve(tx_buf_handle_t const txb, size_t const header_len);
uint8_t * tx_buf_put(tx_buf_handle_t const txb, size_t const user_data_len);
uint8_t * tx_buf_push(tx_buf_handle_t const txb, size_t const header_len);
uint8_t * tx_buf_pull(tx_buf_handle_t const txb, size_t const header_len);
size_t tx_buf_print(char const * const tag, tx_buf_handle_t const txb, char * const buf, size_t const buf_size);
void free_txb(tx_buf_handle_t const txb);
