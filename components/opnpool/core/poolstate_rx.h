/**
 * @file poolstate_rx.h
 * @brief Pool state update interface for received network messages
 *
 * @details
 * Declares the function that updates the local pool state from incoming network
 * messages. The implementation in poolstate_rx.cpp dispatches on message type to
 * handler functions for pump, controller, and chlorinator broadcasts.
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

    // forward declarations (to avoid circular dependencies)
struct network_msg_t;
struct poolstate_t;

namespace poolstate_rx {

/**
 * @brief Update pool state from a received network message.
 *
 * @param[in]     msg    Pointer to the received network message.
 * @param[in,out] state  Pointer to the pool state to update.
 * @return               ESP_OK on success, ESP_FAIL if the message type is unhandled.
 */
[[nodiscard]] esp_err_t update_state(network_msg_t const * const msg, poolstate_t * const state);

}  // namespace poolstate_rx

}  // namespace opnpool
}  // namespace esphome