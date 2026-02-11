/**
 * @file opnpool_text_sensor.h
 * @brief Text sensor entity interface for OPNpool component.
 *
 * @details
 * Declares the OpnPoolTextSensor class, which implements ESPHome's text sensor
 * interface for reporting string values (such as schedules, firmware versions,
 * status messages, etc.) from the pool controller to Home Assistant.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/component.h>
#include <esphome/components/text_sensor/text_sensor.h>
#include <string>

#include "core/opnpool_ids.h"

namespace esphome {
namespace opnpool {

class OpnPool;  // Forward declaration only - don't include opnpool.h!

/**
 * @brief Text sensor entity for OPNpool component.
 *
 * @details
 * Extends ESPHome's TextSensor and Component classes to provide a text sensor
 * that tracks string values and only publishes updates when the value changes.
 */
class OpnPoolTextSensor : public text_sensor::TextSensor, public Component {
  public:
    OpnPoolTextSensor() {}

    /**
     * @brief Dumps the configuration and last known state of the text sensor.
     *
     * Called by ESPHome during startup. Set logger for this module to INFO or
     * higher to see output.
     */
    void dump_config();

    /**
     * @brief Publishes the text sensor state to Home Assistant if it has changed.
     *
     * @param[in] value The new text sensor value to be published.
     */
    void publish_value_if_changed(const std::string &value);

  protected:
    /// @brief Tracks the last published state to avoid redundant updates.
    struct last_t {
        bool         valid;  ///< True if a value has been published at least once.
        std::string  value;  ///< The last published text value.
    } last_ = {
        .valid = false,
        .value = ""
    };
};

}  // namespace opnpool
}  // namespace esphome