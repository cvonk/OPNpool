#pragma once
#include <driver/uart.h>

#include "../ipc/ipc.h"
#include "../datalink/datalink_pkt.h"

typedef struct rs485_instance_t * rs485_handle_t;

typedef int (* rs485_available_fnc_t)(void);
typedef int (* rs485_read_bytes_fnc_t)(uint8_t * dst, uint32_t len);
typedef int (* rs485_write_bytes_fnc_t)(uint8_t * src, size_t len);
typedef int (* rs485_write_fnc_t)(uint8_t src);
typedef void (* rs485_flush_fnc_t)(void);
typedef void (* rx485_tx_mode_fnc_t)(bool const tx_enable);
typedef void (* rs485_queue_fnc_t)(rs485_handle_t const handle, datalink_pkt_t const * const pkt);
typedef datalink_pkt_t const * (* rs485_dequeue_fnc_t)(rs485_handle_t const handle);

typedef struct rs485_instance_t {
    rs485_available_fnc_t available;      // bytes available in rx buffer
    rs485_read_bytes_fnc_t read_bytes;    // read bytes from rx buffer
    rs485_write_bytes_fnc_t write_bytes;  // write bytes to tx buffer 
#if 0
    rs485_write_fnc_t write;              // write 1 byte to tx buffer
#endif
    rs485_flush_fnc_t flush;              // wait until all bytes are transmitted
    rx485_tx_mode_fnc_t tx_mode;          // controls RTS pin (for half-duplex)
    rs485_queue_fnc_t queue;              // queue to handle->tx_q
    rs485_dequeue_fnc_t dequeue;          // dequeue from handle->tx_q
    QueueHandle_t tx_q;                   // transmit queue
} rs485_instance_t;

typedef struct  rs485_q_msg_t {
    datalink_pkt_t const * const pkt;
} rs485_q_msg_t;


/* rs485.c */
rs485_handle_t rs485_init(void);
