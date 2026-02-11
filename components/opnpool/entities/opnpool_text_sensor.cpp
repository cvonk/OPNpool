/**
 * @file opnpool_text_sensor.cpp
 * @brief Reports digital pool text sensors to Home Assistant.
 *
 * @details
 * This file implements the OPNpool text sensor integration for ESPHome, enabling the
 * reporting of various pool controller string values (such as schedules, firmware
 * versions, and status messages) to Home Assistant.
 *
 * ESPHome operates in a single-threaded environment, so explicit thread safety measures
 * are not required within the main task context. The maximum number of text sensors is
 * limited by the use of uint8_t for indexing.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/log.h>

#include "opnpool_text_sensor.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

constexpr char TAG[] = "opnpool_text_sensor";

/**
 * @brief Dump the configuration and last known state of the text sensor entity.
 *
 * @details
 * Logs the configuration details for this text sensor, including its ID and last
 * known value (if valid). This information is useful for diagnostics and debugging,
 * providing visibility into the entity's state and configuration at runtime.
 */
void
OpnPoolTextSensor::dump_config()
{
    LOG_TEXT_SENSOR("  ", "Text Sensor", this);
    ESP_LOGCONFIG(TAG, "    Last value: %s", last_.valid ? last_.value.c_str() : "<unknown>");
}

/**
 * @brief Publishes the text sensor state to Home Assistant if it has changed.
 *
 * @details
 * Compares the new text sensor value with the last published value. If the state is not
 * yet valid, or if the new value differs from the last value, updates the internal state
 * and publishes the new value to Home Assistant. This avoids redundant updates to Home
 * Assistant.
 *
 * @param[in] value The new text sensor value to be published.
 */
void
OpnPoolTextSensor::publish_value_if_changed(const std::string & value)
{
    if (!last_.valid || last_.value != value) {

        this->publish_state(value);
        last_ = {
            .valid = true,
            .value = value
        };
        ESP_LOGV(TAG, "Published %s", value.c_str());
    }
}

}  // namespace opnpool
}  // namespace esphome