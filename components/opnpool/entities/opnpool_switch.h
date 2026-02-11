/**
 * @file opnpool_switch.h
 * @brief Switch entity interface for OPNpool component.
 *
 * @details
 * Declares the OpnPoolSwitch class, which implements ESPHome's switch interface
 * for controlling pool circuits (POOL, SPA, AUX1-3, FEATURE1-4) and publishing
 * their states to Home Assistant.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/component.h>
#include <esphome/components/switch/switch.h>

#include "core/opnpool_ids.h"

namespace esphome {
namespace opnpool {

// Forward declarations
struct poolstate_t;
class OpnPool;

/**
 * @brief Switch entity for OPNpool component.
 *
 * @details
 * Extends ESPHome's Switch and Component classes to provide control over pool
 * circuits. Sends commands to the pool controller via RS-485 and publishes
 * state updates only when the controller confirms the change.
 */
class OpnPoolSwitch : public switch_::Switch, public Component {
  public:
    /**
     * @brief Constructs an OpnPoolSwitch entity.
     *
     * @param[in] parent Pointer to the parent OpnPool component.
     * @param[in] id     The switch entity ID from switch_id_t.
     */
    OpnPoolSwitch(OpnPool* parent, uint8_t id) : parent_{parent}, id_{static_cast<switch_id_t>(id)} {}

    /**
     * @brief Dumps the configuration and last known state of the switch.
     *
     * Called by ESPHome during startup. Set logger for this module to INFO or
     * higher to see output.
     */
    void dump_config();

    /**
     * @brief Handles switch state changes triggered by Home Assistant.
     *
     * @param[in] state The desired state (true for ON, false for OFF).
     */
    void write_state(bool state) override;

    /**
     * @brief Publishes the switch state to Home Assistant if it has changed.
     *
     * @param[in] new_value The new state (true for ON, false for OFF).
     */
    void publish_value_if_changed(bool const new_value);

  protected:
    OpnPool * const              parent_;   ///< Parent OpnPool component.
    switch_id_t const            id_;       ///< Switch entity ID.
    network_pool_circuit_t const circuit_ = switch_id_to_network_circuit(id_);  ///< Mapped circuit type.

    /// @brief Tracks the last published state to avoid redundant updates.
    struct last_t {
        bool valid;  ///< True if a state has been published at least once.
        bool value;  ///< The last published switch state.
    } last_ = {
        .valid = false,
        .value = false
    };
};

}  // namespace opnpool
}  // namespace esphome
