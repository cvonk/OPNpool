#pragma once
#include <driver/uart.h>

#include "../ipc/ipc.h"

typedef struct rs485_config_t {
    int rx_pin;
    int tx_pin;
    int rts_pin;
    uart_port_t uart_port;
    uart_config_t uart_config;
} rs485_config_t;

typedef struct rs485_instance_t * rs485_handle_t;

typedef int (* rs485_available_fnc_t)(void);
typedef int (* rs485_read_bytes_fnc_t)(uint8_t * dst, uint32_t len);
typedef int (* rs485_read_fnc_t)(void);
typedef int (* rs485_write_bytes_fnc_t)(uint8_t * src, size_t len);
typedef int (* rs485_write_fnc_t)(uint8_t src);
typedef void (* rs485_flush_fnc_t)(void);
typedef void (* rs485_queue_fnc_t)(rs485_handle_t const handle, tx_buf_handle_t const txb);
typedef tx_buf_handle_t (* rs485_dequeue_fnc_t)(rs485_handle_t const handle);

typedef struct rs485_instance_t {
    rs485_available_fnc_t available;
    rs485_read_bytes_fnc_t read_bytes;
    rs485_read_fnc_t read;
    rs485_write_bytes_fnc_t write_bytes;
    rs485_write_fnc_t write;
    rs485_flush_fnc_t flush;
    rs485_queue_fnc_t queue;
    rs485_dequeue_fnc_t dequeue;
    QueueHandle_t tx_q;
} rs485_instance_t;

typedef enum RS485_DIR_t {
    RS485_DIR_rx = 0,
    RS485_DIR_tx = 1
} RS485_DIR_t;

/* rs485.c */
rs485_handle_t rs485_init(rs485_config_t const * const cfg);
