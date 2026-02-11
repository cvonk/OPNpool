/**
 * @file ipc.h
 * @brief Inter-Process Communication (IPC) types and function prototypes
 *
 * @details
 * Declares the IPC structures and function prototypes used for message passing
 * between FreeRTOS tasks in the OPNpool component. The main task and pool task
 * communicate via FreeRTOS queues carrying network messages.
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
#include <freertos/queue.h>

#include "core/opnpool.h" // for rs485_pins_t

namespace esphome {
namespace opnpool {

    // forward declarations (to avoid circular dependencies)
struct ipc_t;
struct network_msg_t;

/// @brief Configuration for the pool task.
struct config_t {
    rs485_pins_t rs485_pins;  ///< RS-485 pin assignments.
};

/// @brief IPC context holding FreeRTOS queues and configuration.
struct ipc_t {
    QueueHandle_t to_main_q;  ///< Queue for messages from pool task to main task.
    QueueHandle_t to_pool_q;  ///< Queue for messages from main task to pool task.
    config_t      config;     ///< Pool task configuration.
};

    // function prototypes for ipc.cpp
esp_err_t ipc_send_network_msg_to_main_task(network_msg_t const * const network_msg, ipc_t const * const ipc);
esp_err_t ipc_send_network_msg_to_pool_task(network_msg_t const * const network_msg, ipc_t const * const ipc);

} // namespace opnpool
} // namespace esphome