/**
 * @file rs485.cpp
 * @brief RS485 driver: receive/send bytes to/from the RS485 transceiver
 *
 * @details
 * This file implements the RS485 hardware driver for the OPNpool component, providing
 * low-level functions to initialize, configure, and operate the RS485 transceiver. It
 * handles UART setup for half-duplex communication, GPIO configuration, and manages a
 * transmit queue for outgoing packets. The driver exposes a handle with function pointers
 * for higher-level protocol layers to interact with the RS485 interface, ensuring
 * reliable and efficient communication with pool equipment over the RS485 bus.
 *
 * The driver provides two key functions:
 * 1. Reading bytes from the RS-485 transceiver.
 * 2. Queueing outgoing byte streams, and dequeuing them to write the bytes to the RS-485
 *    transceiver.
 * 
 * ESPHome operates in a single-threaded environment, so explicit thread safety measures
 * are not required within the pool_task context.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2014, 2019, 2022, 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esphome/core/log.h>
#include <esp_rom_sys.h>
#include <string.h>

#include "rs485.h"
#include "datalink.h"
#include "datalink_pkt.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

constexpr char TAG[] = "rs485";

constexpr size_t      RX_BUF_SIZE = 127;
constexpr TickType_t  RX_TIMEOUT  = (100 / portTICK_PERIOD_MS);
constexpr TickType_t  TX_TIMEOUT  = (100 / portTICK_PERIOD_MS);

constexpr uart_port_t           UART_PORT = static_cast<uart_port_t>(UART_NUM_1);
constexpr int                   BAUD_RATE = 9600;
constexpr uart_word_length_t    DATA_BITS = UART_DATA_8_BITS;
constexpr uart_parity_t         PARITY    = UART_PARITY_DISABLE;
constexpr uart_stop_bits_t      STOP_BITS = UART_STOP_BITS_1;
constexpr uart_hw_flowcontrol_t FLOW_CTRL = UART_HW_FLOWCTRL_DISABLE; 
constexpr uart_sclk_t           CLOCK_SRC = UART_SCLK_DEFAULT;
constexpr uint8_t               RX_FLOW_CTRL_THRESH = 122;

static gpio_num_t  _rts_pin;

/**
 * @brief Returns the number of bytes available in the UART RX buffer.
 */
static int
_available()
{
    size_t length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_PORT, &length));
    return static_cast<int>(length);
}

/**
 * @brief          Reads bytes from the UART RX buffer with a timeout.
 *
 * @param[out] dst Destination buffer.
 * @param[in]  len Number of bytes to read.
 * @return         Number of bytes read.
 */
[[nodiscard]] static int
_read_bytes(uint8_t * dst, uint32_t len)
{
    return uart_read_bytes(UART_PORT, dst, len, RX_TIMEOUT);
}

/**
 * @brief         Writes bytes to the UART TX buffer.
 *
 * @param[in] src Source buffer.
 * @param[in] len Number of bytes to write.
 * @return        Number of bytes written.
 */
[[nodiscard]] static int
_write_bytes(uint8_t * src, size_t len)
{
    return uart_write_bytes(UART_PORT, (char *) src, len);
}

/**
 * @brief Flushes the UART TX and RX buffers, waiting for TX to complete.
 */
static void
_flush(void)
{
    ESP_ERROR_CHECK(uart_wait_tx_done(UART_PORT, TX_TIMEOUT));
    ESP_ERROR_CHECK(uart_flush_input(UART_PORT));
}

/**
 * @brief            Queues a packet for transmission on the RS-485 bus.
 *
 * @param[in] handle RS-485 handle.
 * @param[in] pkt    Packet to queue.
 */
static void
_queue(rs485_handle_t const handle, datalink_pkt_t const * const pkt)
{
    if (pkt != nullptr) {
        rs485_q_msg_t msg = {
            .pkt = pkt,
        };
        if (xQueueSendToBack(handle->tx_q, &msg, 0) != pdPASS) {
            ESP_LOGE(TAG, "tx_q full");
            free(pkt->skb);
            free((void *) pkt);
        }
    }
}

/**
 * @brief            Dequeues a packet from the RS-485 transmit queue.
 *
 * @param[in] handle RS-485 handle.
 * @return           Pointer to the dequeued packet, or NULL if none available.
 */
[[nodiscard]] static datalink_pkt_t const *
_dequeue(rs485_handle_t const handle)
{
    rs485_q_msg_t msg{};
    if (xQueueReceive(handle->tx_q, &msg, (TickType_t)0) == pdPASS) {
        if (msg.pkt == nullptr) {
            ESP_LOGE(TAG, "Dequeued packet is null");
        }
        return msg.pkt;
    }
    return nullptr;
}

/**
 * @brief               Sets the RS-485 transceiver to transmit or receive mode.
 *
 * @param[in] tx_enable True to enable transmit mode, false for receive mode.
 */
static void
_tx_mode(bool const tx_enable)
{
    // messages should be sent directly after an A5 packets (and before any IC packets)
    // A note on the DE signal:
    //  - choose a GPIO that doesn't mind being pulled down during reset

    if (tx_enable) {
        gpio_set_level(_rts_pin, 1);  // enable RS485 transmit DE=1 and RE*=1 (DE=driver enable, RE*=inverted receive enable)
    } else {
        _flush();                     // wait until last byte starts transmitting
        esp_rom_delay_us(1500);       // wait until last byte is transmitted (10 bits / 9600 baud =~ 1042 µs)
        gpio_set_level(_rts_pin, 0);  // enable RS485 receive
    }
}

/**
 * @brief Initializes the RS485 hardware interface and driver.
 *
 * @details
 * Configures the specified UART port and GPIO pins for RS485 half-duplex communication,
 * sets up the UART parameters (baud rate, data bits, stop bits, etc.), and initializes
 * the request-to-send (RTS) pin for transmit/receive direction control. Allocates and
 * initializes the RS485 handle structure, sets up the transmit queue, and assigns
 * function pointers for RS485 operations. Returns a handle to the initialized RS485
 * interface for use by higher-level protocol layers.
 *
 * @param[in] rs485_pins Pointer to the structure containing RX, TX, and RTS pin numbers.
 * @return               Handle to the initialized RS485 interface.
 */
[[nodiscard]] rs485_handle_t
rs485_init(rs485_pins_t const * const rs485_pins)
{
    gpio_num_t const rx_pin = static_cast<gpio_num_t>(rs485_pins->rx_pin);
    gpio_num_t const tx_pin = static_cast<gpio_num_t>(rs485_pins->tx_pin);
    _rts_pin = static_cast<gpio_num_t>(rs485_pins->rts_pin);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"    
    uart_config_t const uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = DATA_BITS,
        .parity = PARITY,
        .stop_bits = STOP_BITS,
        .flow_ctrl = FLOW_CTRL,
        .rx_flow_ctrl_thresh = RX_FLOW_CTRL_THRESH,
        .source_clk = CLOCK_SRC,
        .flags = {
            .backup_before_sleep = 0
        }
    };
#pragma GCC diagnostic pop

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << static_cast<uint8_t>(_rts_pin)),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK( gpio_config(&io_conf) );
    gpio_set_level(_rts_pin, 0);

    ESP_LOGI(TAG, "Initializing RS485 on UART%u (RX pin %u, TX pin %u, RTS pin %u) ..",
             UART_PORT, rx_pin, tx_pin, _rts_pin);

    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, RX_BUF_SIZE * 2, 0, 0, NULL, 0);  // no tx buffer
    uart_set_mode(UART_PORT, UART_MODE_RS485_HALF_DUPLEX);

    QueueHandle_t const tx_q = xQueueCreate(5, sizeof(rs485_q_msg_t));
    if (tx_q == nullptr) {
        ESP_LOGE(TAG, "Failed to create TX queue");
        return nullptr;
    }

    rs485_handle_t handle = static_cast<rs485_handle_t>(calloc(1, sizeof(rs485_instance_t)));
    if (handle == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate RS485 handle");
        vQueueDelete(tx_q);
        return nullptr;
    }

    handle->available = _available;
    handle->read_bytes = _read_bytes;
    handle->write_bytes = _write_bytes;
    handle->flush = _flush;
    handle->tx_mode = _tx_mode;
    handle->queue = _queue;
    handle->dequeue = _dequeue;
    handle->tx_q = tx_q;
    
    _tx_mode(false);

    return handle;
}

}  // namespace opnpool
}  // namespace esphome