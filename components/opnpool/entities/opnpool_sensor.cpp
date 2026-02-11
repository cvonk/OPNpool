/**
 * @file opnpool_sensor.cpp
 * @brief Reports analog pool sensors to Home Assistant.
 *
 * @details
 * Implements the sensor entity interface for the OPNpool component, allowing ESPHome to
 * monitor pool-related sensor values (such as temperatures, etc.) and publish them to
 * Home Assistant.
 *
 * ESPHome operates in a single-threaded environment, so explicit thread safety measures
 * are not required within the main task context. The maximum number of sensors is
 * limited by the use of uint8_t for indexing.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/log.h>

#include "opnpool_sensor.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

constexpr char TAG[] = "opnpool_sensor";

/**
 * @brief Dump the configuration and last known state of the sensor entity.
 *
 * @details
 * Logs the configuration details for this analog sensor, including its ID and last
 * known value (if valid). This information is useful for diagnostics and debugging,
 * providing visibility into the entity's state and configuration at runtime.
 */
void
OpnPoolSensor::dump_config()
{
    LOG_SENSOR("  ", "Sensor", this);
    ESP_LOGCONFIG(TAG, "    Last value: %s", last_.valid ? std::to_string(last_.value).c_str() : "<unknown>");
}

/**
 * @brief Publishes the sensor state to Home Assistant if it has changed.
 *
 * @details
 * Compares the new sensor value with the last published value. If the state is not yet
 * valid, or if the absolute difference between the new value and the last value exceeds
 * the given tolerance, updates the internal state and publishes the new value to Home
 * Assistant. This avoids redundant updates to Home Assistant.
 *
 * @param[in] value     The new sensor value to be published.
 * @param[in] tolerance The minimum change required to trigger a new state publication.
 */
void
OpnPoolSensor::publish_value_if_changed(float value, float tolerance)
{
    if (!last_.valid || fabs(last_.value - value) > tolerance) {

        this->publish_state(value);
        
        last_ = {
            .valid = true,
            .value = value
        };
        ESP_LOGV(TAG, "Published %.1f", value);        
    }
}

}  // namespace opnpool
}  // namespace esphome