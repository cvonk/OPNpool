/**
 * @file opnpool_ids.h
 * @brief Entity ID enumerations and conversion utilities for OPNpool component.
 *
 * @details
 * Defines entity ID enumerations used for array indexing of ESPHome entities
 * (climates, switches, sensors, binary sensors, text sensors). Also declares
 * conversion functions that map these IDs to their corresponding internal
 * representations used by the pool controller protocol.
 *
 * @note The enum values may be overwritten by __init__.py to ensure consistency
 *       with ESPHome's CONF_* configuration constants.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#ifndef ESPHOME_OPNPOOL_IDS_H_
#define ESPHOME_OPNPOOL_IDS_H_

#include <esp_system.h>
#include <esp_types.h>

namespace esphome {
namespace opnpool {

    // forward declarations (to avoid circular dependencies)
enum class poolstate_thermo_typ_t : uint8_t;
enum class network_pool_circuit_t : uint8_t;

    /// @brief Climate entity identifiers for pool and spa thermostats.
enum class climate_id_t : uint8_t {
    POOL_CLIMATE = 0,  ///< Pool thermostat climate entity.
    SPA_CLIMATE  = 1   ///< Spa thermostat climate entity.
};

   /// @brief Switch entity identifiers for pool circuit controls.
enum class switch_id_t : uint8_t {
    SPA      = 0,  ///< Spa circuit switch.
    AUX1     = 1,  ///< Auxiliary circuit 1 switch.
    AUX2     = 2,  ///< Auxiliary circuit 2 switch.
    AUX3     = 3,  ///< Auxiliary circuit 3 switch.
    FEATURE1 = 4,  ///< Feature circuit 1 switch.
    POOL     = 5,  ///< Pool circuit switch.
    FEATURE2 = 6,  ///< Feature circuit 2 switch.
    FEATURE3 = 7,  ///< Feature circuit 3 switch.
    FEATURE4 = 8   ///< Feature circuit 4 switch.
};

    /// @brief Sensor entity identifiers for pool measurements.
enum class sensor_id_t : uint8_t {
    AIR_TEMPERATURE    = 0,  ///< Ambient air temperature sensor.
    WATER_TEMPERATURE  = 1,  ///< Pool/spa water temperature sensor.
    PRIMARY_PUMP_POWER = 2,  ///< Primary pump power consumption sensor.
    PRIMARY_PUMP_FLOW  = 3,  ///< Primary pump flow rate sensor.
    PRIMARY_PUMP_SPEED = 4,  ///< Primary pump speed (RPM) sensor.
    CHLORINATOR_LEVEL  = 5,  ///< Chlorinator output level sensor.
    CHLORINATOR_SALT   = 6,  ///< Chlorinator salt level sensor.
    PRIMARY_PUMP_ERROR = 7   ///< Primary pump error code sensor.
};

    /// @brief Binary sensor entity identifiers for pool status indicators.
enum class binary_sensor_id_t : uint8_t {
    PRIMARY_PUMP_POWER   = 0,  ///< Primary pump running status.
    MODE_SERVICE           = 1,  ///< Service mode active indicator.
    MODE_TEMPERATURE_INC   = 2,  ///< Temperature increase mode indicator.
    MODE_FREEZE_PROTECTION = 3,  ///< Freeze protection mode active indicator.
    MODE_TIMEOUT           = 4   ///< Timeout mode active indicator.
};

    /// @brief Text sensor entity identifiers for pool status strings.
enum class text_sensor_id_t : uint8_t {
    POOL_SCHED          = 0,  ///< Pool schedule information.
    SPA_SCHED           = 1,  ///< Spa schedule information.
    PRIMARY_PUMP_MODE   = 2,  ///< Primary pump operating mode.
    PRIMARY_PUMP_STATE  = 3,  ///< Primary pump state description.
    CHLORINATOR_NAME    = 4,  ///< Chlorinator device name.
    CHLORINATOR_STATUS  = 5,  ///< Chlorinator status description.
    SYSTEM_TIME         = 6,  ///< Pool controller system time.
    CONTROLLER_TYPE = 7,  ///< Pool controller firmware version.
    INTERFACE_FIRMWARE  = 8   ///< Interface board firmware version.
};

/**
 * @brief Converts a climate entity ID to the corresponding thermostat type.
 *
 * @param[in] id The climate entity ID to convert.
 * @return       The corresponding poolstate_thermo_typ_t value.
 */
[[nodiscard]] poolstate_thermo_typ_t climate_id_to_poolstate_thermo(climate_id_t const id);

/**
 * @brief Converts a switch entity ID to the corresponding pool circuit type.
 *
 * @param[in] id The switch entity ID to convert.
 * @return       The corresponding network_pool_circuit_t value.
 */
[[nodiscard]] network_pool_circuit_t switch_id_to_network_circuit(switch_id_t const id);

} // namespace opnpool
} // namespace esphome

#endif // ESPHOME_OPNPOOL_IDS_H_