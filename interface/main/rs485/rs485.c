/**
 * @brief RS485 driver: receive/sent bytes to/from the RS485 transceiver
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
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

static char const * const TAG = "rs485";

static size_t     _rxBufSize = 127;
static TickType_t _rxTimeout = (100 / portTICK_RATE_MS);
static TickType_t _txTimeout = (100 / portTICK_RATE_MS);

static uart_port_t _uart_port;

typedef struct tx_msg_t {
    tx_buf_handle_t  txb;  // must be freed by recipient
} tx_msg_t;

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

static int
_read(void)
{
    uint8_t dst;
    _read_bytes(&dst, 1);
    return dst;
}

static int
_write(uint8_t src)
{
    return uart_write_bytes(_uart_port, (char *) &src, 1);
}

static int
_write_bytes(uint8_t * src, size_t len)
{
#if 0
	// messages should be sent directly after an A5 packets (and before any IC packets)

	// enable RS485 transmit DE=1 and RE*=1 (DE=driver enable, RE*=inverted receive enable)
	//CJV?? digitalWrite(dirPin, RS485_DIR_tx);  // 2BD: there might be a mandatory wait after enabling this pin !!!!!!!
    gpio_set_level(GPIO_NUM_27, 1);
	{
		rs485->write(0xFF);
		for (uint_least8_t ii = 0; ii < sizeof(datalink_preamble_a5); ii++) {
			rs485->write(datalink_preamble_a5[ii]);
		}
		for (uint_least8_t ii = 0; ii < sizeof(mHdr_a5_t); ii++) {
			rs485->write(((uint8_t *)&hdr)[ii]);
		}

		printf("DBG typ=%02X dataLen=%u data.circuitSet=(%02X %02X)", element->typ, element->dataLen, element->data.circuitSet.circuit, element->data.circuitSet.value);
		for (uint_least8_t ii = 0; ii < hdr.len; ii++) {
			printf(" %02X", element->data.raw[ii]);
			rs485->write(element->data.raw[ii]);
		}
		printf("\n");
		rs485->write(crc >> 8);
		rs485->write(crc & 0xFF);
		//rs485->write(0xFF);
		rs485->flush();  // wait until the hardware buffer starts transmitting the last byte

		// A few words on the DE signal:
		//  - choose a GPIO that doesn't mind being pulled down during reset
		//  - in an ideal world, the UART has a tx interrupt, or at least a tx-complete bit, so
		//    that a transmit-done callback can off the DE.
		//  - probing the TX and DE signal, I learned that the DE is 1 byte time (10/9600 sec) to
		//    short.  Correct with a delay() statement.

		//vTaskDelay(1/portTICK_PERIOD_MS);
	}
	ets_delay_us(1500);
    gpio_set_level(GPIO_NUM_27, 0);  // enable RS485 receive
#endif

    return uart_write_bytes(_uart_port, (char *) src, len);
}

static void
_flush(void)
{
    ESP_ERROR_CHECK(uart_wait_tx_done(_uart_port, _txTimeout));
    ESP_ERROR_CHECK(uart_flush_input(_uart_port));
}

static void
_queue(rs485_handle_t const handle, tx_buf_handle_t const txb)
{
    ESP_LOGW(TAG, "5 head=%p data=%p tail=%p, end=%p len=%u q=%p", txb->priv.head, txb->priv.data, txb->priv.tail, txb->priv.end, txb->len, handle);
    tx_msg_t msg = {
        .txb = txb, // doesn't copy the data
    };
    assert(msg.txb);
    if (xQueueSendToBack(handle->tx_q, &msg, 0) != pdPASS) {
        ESP_LOGE(TAG, "tx_q full");
        free(msg.txb);
    }
}

static tx_buf_handle_t
_dequeue(rs485_handle_t const handle)
{
    tx_msg_t msg;
    if (xQueueReceive(handle->tx_q, &msg, (TickType_t)0) == pdPASS) {
        return msg.txb;
    }
    return NULL;
}

rs485_handle_t
rs485_init(rs485_config_t const * const cfg)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << cfg->rts_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK( gpio_config(&io_conf) );
    gpio_set_level((gpio_num_t)cfg->rts_pin, 0);

    uart_param_config(cfg->uart_port, &cfg->uart_config);
    uart_set_pin(cfg->uart_port, cfg->tx_pin, cfg->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(cfg->uart_port, _rxBufSize * 2, 0, 0, NULL, 0);  // no tx buffer
    uart_set_mode(cfg->uart_port, UART_MODE_RS485_HALF_DUPLEX);

    QueueHandle_t const tx_q = xQueueCreate(2, sizeof(tx_msg_t));
    assert(tx_q);

    rs485_handle_t handle = malloc(sizeof(rs485_instance_t));
    assert(handle);

    _uart_port = cfg->uart_port;
    handle->available = _available;
    handle->read_bytes = _read_bytes;
    handle->read = _read;
    handle->write_bytes = _write_bytes;
    handle->write = _write;
    handle->flush = _flush;
    handle->queue = _queue;
    handle->dequeue = _dequeue;
    handle->tx_q = tx_q;

    return handle;
}