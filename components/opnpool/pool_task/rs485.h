/**
 * @file rs485.h
 * @brief RS-485 driver interface for pool controller communication.
 *
 * @details
 * Declares the RS-485 hardware driver interface for the OPNpool component. Provides
 * type definitions for function pointers used by the driver handle, the instance
 * structure that encapsulates all RS-485 operations, and the transmit queue message
 * structure. This abstraction allows higher-level protocol layers (datalink, network)
 * to interact with the RS-485 interface without direct hardware dependencies.
 *
 * The driver supports half-duplex communication with RTS-based direction control,
 * suitable for RS-485 transceivers connected to pool equipment.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2014, 2019, 2022, 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#ifndef __cplusplus
# error "Requires C++ compilation"
#endif

#include <esp_system.h>
#include <esp_types.h>
#include <driver/uart.h>
#include <cstddef>

#include "ipc/ipc.h"

namespace esphome {
namespace opnpool {

/// @name Forward Declarations
/// @brief Avoids circular dependencies with datalink types.
/// @{
struct datalink_pkt_t;
/// @}

/// @name Type Aliases
/// @brief Handle type for the RS-485 driver instance.
/// @{
using rs485_handle_t = struct rs485_instance_t *;
/// @}

/// @name Function Pointer Types
/// @brief Type definitions for RS-485 driver operations.
/// @{

/// @brief Function pointer: returns number of bytes available in RX buffer.
using rs485_available_fnc_t   = int (*)(void);

/// @brief Function pointer: reads bytes from the RX buffer into destination.
using rs485_read_bytes_fnc_t  = int (*)(uint8_t * dst, uint32_t len);

/// @brief Function pointer: writes bytes from source to the TX buffer.
using rs485_write_bytes_fnc_t = int (*)(uint8_t * src, size_t len);

/// @brief Function pointer: flushes TX buffer and waits for transmission to complete.
using rs485_flush_fnc_t       = void (*)(void);

/// @brief Function pointer: sets transmit/receive mode via RTS pin.
using rs485_tx_mode_fnc_t     = void (*)(bool const tx_enable);

/// @brief Function pointer: queues a packet for transmission.
using rs485_queue_fnc_t       = void (*)(rs485_handle_t const handle, datalink_pkt_t const * const pkt);

/// @brief Function pointer: dequeues a packet from the transmit queue.
using rs485_dequeue_fnc_t     = datalink_pkt_t const * (*)(rs485_handle_t const handle);

/// @}

/// @name RS-485 Instance Structure
/// @brief Main structure encapsulating RS-485 driver state and operations.
/// @{

/**
 * @brief RS-485 driver instance structure.
 *
 * @details
 * Contains function pointers for all RS-485 operations and the transmit queue handle.
 * This structure is allocated and initialized by rs485_init() and provides a unified
 * interface for higher-level protocol layers to interact with the RS-485 hardware.
 */
struct rs485_instance_t {
    rs485_available_fnc_t   available;    ///< Returns bytes available in RX buffer.
    rs485_read_bytes_fnc_t  read_bytes;   ///< Reads bytes from RX buffer.
    rs485_write_bytes_fnc_t write_bytes;  ///< Writes bytes to TX buffer.
    rs485_flush_fnc_t       flush;        ///< Waits until all bytes are transmitted.
    rs485_tx_mode_fnc_t     tx_mode;      ///< Controls RTS pin for half-duplex direction.
    rs485_queue_fnc_t       queue;        ///< Queues packet to tx_q for transmission.
    rs485_dequeue_fnc_t     dequeue;      ///< Dequeues packet from tx_q.
    QueueHandle_t           tx_q;         ///< FreeRTOS transmit queue handle.
};

/// @}

/// @name Transmit Queue Message
/// @brief Structure for messages in the RS-485 transmit queue.
/// @{

/**
 * @brief Transmit queue message structure.
 *
 * @details
 * Wraps a datalink packet pointer for queuing in the FreeRTOS transmit queue.
 * The packet pointer is const to prevent modification after queueing.
 */
struct rs485_q_msg_t {
    datalink_pkt_t const * const pkt;  ///< Pointer to the packet (requires initialization at construction).
};

/// @}

/// @name Driver Initialization
/// @brief Function to initialize the RS-485 hardware interface.
/// @{

/**
 * @brief Initializes the RS-485 hardware interface and driver.
 *
 * @param[in] rs485_pins Pointer to the structure containing RX, TX, and RTS pin numbers.
 * @return               Handle to the initialized RS-485 interface, or nullptr on failure.
 */
[[nodiscard]] rs485_handle_t rs485_init(rs485_pins_t const * const rs485_pins);

/// @}

}  // namespace opnpool
}  // namespace esphome
