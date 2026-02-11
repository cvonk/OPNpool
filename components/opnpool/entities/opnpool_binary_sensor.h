/**
 * @file opnpool_binary_sensor.h
 * @brief Binary sensor entity interface for OPNpool component.
 *
 * @details
 * Declares the OpnPoolBinarySensor class, which implements ESPHome's binary sensor
 * interface for monitoring digital pool sensor states (such as pump status, error
 * conditions, etc.) and publishing them to Home Assistant.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/component.h>
#include <esphome/components/binary_sensor/binary_sensor.h>

#include "core/opnpool_ids.h"

namespace esphome {
namespace opnpool {

/**
 * @brief Binary sensor entity for OPNpool component.
 *
 * @details
 * Extends ESPHome's BinarySensor and Component classes to provide a binary sensor
 * that tracks ON/OFF states and only publishes updates when the value changes.
 */
class OpnPoolBinarySensor : public binary_sensor::BinarySensor, public Component {

  public:
    OpnPoolBinarySensor() {}

    /**
     * @brief Dumps the configuration and last known state of the binary sensor.
     *
     * Called by ESPHome during startup. Set logger for this module to INFO or
     * higher to see output.
     */
    void dump_config();

    /**
     * @brief Publishes the binary sensor state to Home Assistant if it has changed.
     *
     * @param[in] value The new binary sensor value to be published.
     */
    void publish_value_if_changed(bool value);

  protected:
        /// @brief Tracks the last published state to avoid redundant updates
    struct last_t {
        bool valid;  ///< True if a value has been published at least once.
        bool value;  ///< The last published binary sensor value
    } last_ = {
        .valid = false,
        .value = false
    };
};

}  // namespace opnpool
}  // namespace esphome