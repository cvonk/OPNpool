/**
 * @file opnpool_sensor.h
 * @brief Sensor entity interface for OPNpool component.
 *
 * @details
 * Declares the OpnPoolSensor class, which implements ESPHome's sensor interface
 * for monitoring analog pool sensor values (such as temperatures, pump metrics,
 * chlorinator levels, etc.) and publishing them to Home Assistant.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/component.h>
#include <esphome/components/sensor/sensor.h>

#include "core/opnpool_ids.h"

namespace esphome {
namespace opnpool {

/**
 * @brief Sensor entity for OPNpool component.
 *
 * @details
 * Extends ESPHome's Sensor and Component classes to provide an analog sensor
 * that tracks numeric values and only publishes updates when the value changes
 * beyond a configurable tolerance threshold.
 */
class OpnPoolSensor : public sensor::Sensor, public Component {

  public:
    OpnPoolSensor() {}

    /**
     * @brief Dumps the configuration and last known state of the sensor.
     *
     * Called by ESPHome during startup. Set logger for this module to INFO or
     * higher to see output.
     */
    void dump_config();

    /**
     * @brief Publishes the sensor state to Home Assistant if it has changed.
     *
     * @param[in] value     The new sensor value to be published.
     * @param[in] tolerance The minimum change required to trigger publication.
     */
    void publish_value_if_changed(float value, float tolerance = 0.01f);

  protected:
    /// @brief Tracks the last published state to avoid redundant updates.
    struct last_t {
        bool  valid;  ///< True if a value has been published at least once.
        float value;  ///< The last published sensor value.
    } last_ = {
        .valid = false,
        .value = 0.0f
    };
};

}  // namespace opnpool
}  // namespace esphome