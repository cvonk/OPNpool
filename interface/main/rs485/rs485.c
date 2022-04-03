/**
 * @brief RS485 driver: receive/sent bytes to/from the RS485 transceiver
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020-2022, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/queue.h>

#include "rs485.h"
#include "../datalink/datalink.h"
#include "../datalink/datalink_pkt.h"

static char const * const TAG = "rs485";

static size_t     _rxBufSize = 127;
static TickType_t _rxTimeout = (100 / portTICK_RATE_MS);
static TickType_t _txTimeout = (100 / portTICK_RATE_MS);

static uart_port_t _uart_port;

static int
_available()
{
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(_uart_port, (size_t *) &length));
    return length;
}

static int
_read_bytes(uint8_t * dst, uint32_t len)
{
    return uart_read_bytes(_uart_port, dst, len, _rxTimeout);
}

#if 0
static int
_write(uint8_t src)
{
    return uart_write_bytes(_uart_port, (char *) &src, 1);
}
#endif

static int
_write_bytes(uint8_t * src, size_t len)
{
    return uart_write_bytes(_uart_port, (char *) src, len);
}

static void
_flush(void)
{
    ESP_ERROR_CHECK(uart_wait_tx_done(_uart_port, _txTimeout));
    ESP_ERROR_CHECK(uart_flush_input(_uart_port));
}

static void
_queue(rs485_handle_t const handle, datalink_pkt_t const * const pkt)
{
    assert(pkt);
    rs485_q_msg_t msg = {
        .pkt = pkt,
    };
    if (xQueueSendToBack(handle->tx_q, &msg, 0) != pdPASS) {
        ESP_LOGE(TAG, "tx_q full");
        free(pkt->skb);
        free((void *) pkt);
    }
}

static datalink_pkt_t const *
_dequeue(rs485_handle_t const handle)
{
    rs485_q_msg_t msg;
    if (xQueueReceive(handle->tx_q, &msg, (TickType_t)0) == pdPASS) {
        assert(msg.pkt);
        return msg.pkt;
    }
    return NULL;
}

static void
_tx_mode(bool const tx_enable)
{
	// messages should be sent directly after an A5 packets (and before any IC packets)
	// 2BD: there might be a mandatory wait after enabling this pin !!!!!!!
    // A few words on the DE signal:
    //  - choose a GPIO that doesn't mind being pulled down during reset

    if (tx_enable) {
        gpio_set_level(CONFIG_OPNPOOL_RS485_RTSPIN, 1);  // enable RS485 transmit DE=1 and RE*=1 (DE=driver enable, RE*=inverted receive enable)
    } else {
        _flush();  // wait until last byte starts transmitting
        ets_delay_us(1500);  // wait until last byte is transmitted (10 bits / 9600 baud =~ 1042 ms)
        gpio_set_level(CONFIG_OPNPOOL_RS485_RTSPIN, 0);  // enable RS485 receive
     }
}

rs485_handle_t
rs485_init(void)
{
    _uart_port = 2;

    uart_config_t const uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .use_ref_tick = false,
    };
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_OPNPOOL_RS485_RTSPIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK( gpio_config(&io_conf) );
    gpio_set_level((gpio_num_t)CONFIG_OPNPOOL_RS485_RTSPIN, 0);

    uart_param_config(_uart_port, &uart_config);
    uart_set_pin(_uart_port, CONFIG_OPNPOOL_RS485_TXPIN, CONFIG_OPNPOOL_RS485_RXPIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(_uart_port, _rxBufSize * 2, 0, 0, NULL, 0);  // no tx buffer
    uart_set_mode(_uart_port, UART_MODE_RS485_HALF_DUPLEX);

    QueueHandle_t const tx_q = xQueueCreate(5, sizeof(rs485_q_msg_t));
    assert(tx_q);

    rs485_handle_t handle = malloc(sizeof(rs485_instance_t));
    assert(handle);

    handle->available = _available;
    handle->read_bytes = _read_bytes;
    handle->write_bytes = _write_bytes;
#if 0
    handle->write = _write;
#endif
    handle->flush = _flush;
    handle->tx_mode = _tx_mode;
    handle->queue = _queue;
    handle->dequeue = _dequeue;
    handle->tx_q = tx_q;
    _tx_mode(false);

    return handle;
}