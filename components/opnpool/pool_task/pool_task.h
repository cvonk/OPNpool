/**
 * @file pool_task.h
 * @brief FreeRTOS task interface for RS-485 pool controller communication.
 *
 * @details
 * Declares the pool_task function, which runs as a FreeRTOS task handling
 * RS-485 communication with the pool controller. The task manages protocol
 * parsing, packet transmission/reception, and IPC with the main ESPHome task.
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

namespace esphome {
namespace opnpool {

/**
 * @brief FreeRTOS task for RS-485 communication and protocol handling.
 *
 * @param[in] ipc_void Pointer to the IPC structure (as void* for FreeRTOS compatibility).
 */
void pool_task(void * ipc_void);

}  // namespace opnpool
}  // namespace esphome