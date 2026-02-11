/**
 * @file matter_endpoint_helpers.h
 * @brief Helper utilities for Matter endpoint management.
 *
 * @details
 * Provides helper functions and utilities for managing Matter endpoints,
 * including attribute value conversion and endpoint configuration helpers.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#ifdef USE_MATTER

#include <esp_matter.h>
#include <esp_matter_attribute_utils.h>

namespace esphome {
namespace opnpool {
namespace matter {

/// @name Attribute Value Helpers
/// @brief Helper functions for creating Matter attribute values.
/// @{

/**
 * @brief Create a nullable int16 attribute value.
 *
 * @param[in] value The int16 value.
 * @return          esp_matter_attr_val_t with the value.
 */
[[nodiscard]] inline esp_matter_attr_val_t
attr_val_int16(int16_t value)
{
    return esp_matter_nullable_int16(value);
}

/**
 * @brief Create a boolean attribute value.
 *
 * @param[in] value The boolean value.
 * @return          esp_matter_attr_val_t with the value.
 */
[[nodiscard]] inline esp_matter_attr_val_t
attr_val_bool(bool value)
{
    return esp_matter_bool(value);
}

/**
 * @brief Create an enum8 attribute value.
 *
 * @param[in] value The enum8 value.
 * @return          esp_matter_attr_val_t with the value.
 */
[[nodiscard]] inline esp_matter_attr_val_t
attr_val_enum8(uint8_t value)
{
    return esp_matter_enum8(value);
}

/**
 * @brief Create a uint16 attribute value.
 *
 * @param[in] value The uint16 value.
 * @return          esp_matter_attr_val_t with the value.
 */
[[nodiscard]] inline esp_matter_attr_val_t
attr_val_uint16(uint16_t value)
{
    return esp_matter_uint16(value);
}

/// @}

/// @name Temperature Conversion Constants
/// @brief Constants for Matter temperature representation.
/// @{

constexpr int16_t MATTER_TEMP_MIN_CENTI_C = -4000;   ///< -40.00°C minimum.
constexpr int16_t MATTER_TEMP_MAX_CENTI_C = 12500;   ///< 125.00°C maximum.
constexpr int16_t MATTER_TEMP_INVALID     = 0x8000;  ///< Invalid/null temperature.

/// @}

/// @name Thermostat Mode Constants
/// @brief Matter Thermostat SystemMode values.
/// @{

constexpr uint8_t THERMOSTAT_MODE_OFF  = 0;  ///< System off.
constexpr uint8_t THERMOSTAT_MODE_AUTO = 1;  ///< Auto mode.
constexpr uint8_t THERMOSTAT_MODE_COOL = 3;  ///< Cooling mode.
constexpr uint8_t THERMOSTAT_MODE_HEAT = 4;  ///< Heating mode.

/// @}

/// @name Thermostat Running State Constants
/// @brief Matter Thermostat ThermostatRunningState values.
/// @{

constexpr uint16_t THERMOSTAT_RUNNING_IDLE    = 0x0000;  ///< Idle (not running).
constexpr uint16_t THERMOSTAT_RUNNING_HEATING = 0x0001;  ///< Heating active.
constexpr uint16_t THERMOSTAT_RUNNING_COOLING = 0x0002;  ///< Cooling active.

/// @}

/**
 * @brief Clamp temperature value to Matter valid range.
 *
 * @param[in] temp_centi_c Temperature in 0.01°C units.
 * @return                 Clamped temperature value.
 */
[[nodiscard]] inline int16_t
clamp_temperature(int16_t temp_centi_c)
{
    if (temp_centi_c < MATTER_TEMP_MIN_CENTI_C) {
        return MATTER_TEMP_MIN_CENTI_C;
    }
    if (temp_centi_c > MATTER_TEMP_MAX_CENTI_C) {
        return MATTER_TEMP_MAX_CENTI_C;
    }
    return temp_centi_c;
}

/**
 * @brief Convert pool heat source to Matter thermostat mode.
 *
 * @param[in] heat_src Pool heat source enum value.
 * @param[in] active   Whether the circuit is active.
 * @return             Matter SystemMode value.
 */
[[nodiscard]] inline uint8_t
heat_src_to_thermostat_mode(uint8_t heat_src, bool active)
{
    if (!active) {
        return THERMOSTAT_MODE_OFF;
    }
    // All heat sources map to HEAT mode in Matter (no cooling for pools)
    return THERMOSTAT_MODE_HEAT;
}

}  // namespace matter
}  // namespace opnpool
}  // namespace esphome

#endif  // USE_MATTER
