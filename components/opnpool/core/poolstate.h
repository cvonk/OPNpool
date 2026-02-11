/**
 * @file poolstate.h
 * @brief Pool State data structures and interface for OPNpool component.
 *
 * @details
 * This header defines the data structures and interface for representing and managing the
 * real-time state of the pool system in the OPNpool component. It maintains a
 * comprehensive software model of the pool controller and all connected peripherals
 * (pump, chlorinator, circuits, sensors, etc.), enabling accurate monitoring and control.
 *
 * The pool state is continuously updated in response to incoming network messages,
 * ensuring that the software state always reflects the latest equipment status and
 * configuration. This layer provides the foundation for publishing sensor values, driving
 * automation logic, and seamless integration with ESPHome and Home Assistant entities. By
 * abstracting hardware details and protocol specifics, it supports modular, robust, and
 * maintainable pool automation.
 *
 * The design supports modular separation of protocol, network, and application logic, and
 * is intended for use in a single-threaded ESPHome environment. Forward declarations are
 * provided to avoid circular dependencies.
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
#include <freertos/FreeRTOS.h>

#if defined(MAGIC_ENUM_RANGE_MIN)
# undef MAGIC_ENUM_RANGE_MIN
#endif
#if defined(MAGIC_ENUM_RANGE_MAX)
# undef MAGIC_ENUM_RANGE_MAX
#endif
#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include "utils/magic_enum.h"
#include "pool_task/network.h"
#include "pool_task/network_msg.h"

namespace esphome {
namespace opnpool {

/// @name Primitive Value Wrappers
/// @brief Type-safe wrappers with validity flags for pool state values.
/// @{

/// @brief Boolean value with validity flag.
struct poolstate_bool_t {
    bool valid;  ///< True if value has been set.
    bool value;  ///< The boolean value.
};

/// @brief 8-bit unsigned value with validity flag.
struct poolstate_uint8_t {
    bool    valid;  ///< True if value has been set.
    uint8_t value;  ///< The 8-bit value.
};

/// @brief 16-bit unsigned value with validity flag.
struct poolstate_uint16_t {
    bool     valid;  ///< True if value has been set.
    uint16_t value;  ///< The 16-bit value.
};

/// @}

/// @name Date and Time Structures
/// @brief Structures for representing time, date, and time-of-day.
/// @{

/// @brief Time value (hour:minute) with validity flag.
struct poolstate_time_t {
    bool           valid;  ///< True if time has been set.
    network_time_t value;
};

/// @brief Date value with validity flag.
struct poolstate_date_t {
    bool           valid;  ///< True if date has been set.
    network_date_t value;
};

/// @brief Combined date and time (time-of-day).
struct poolstate_tod_t {
    poolstate_date_t date;  ///< Date component.
    poolstate_time_t time;  ///< Time component.
};

/// @name Pool Mode Structure
/// @brief Structure for representing pool mode flags.
/// @{

/// @brief Pool mode flags with validity.
struct poolstate_modes_t {
    bool                 valid;  ///< True if mode has been set.
    network_ctrl_modes_t value;  ///< Pool mode flags.
};

/// @}


/// @name System Information Structures
/// @brief Structures for firmware version and system state.
/// @{

/// @brief Firmware version with validity flag.
struct poolstate_version_t {
    bool    valid;  ///< True if version has been set.
    uint8_t major;  ///< Major version number.
    uint8_t minor;  ///< Minor version number.
};

struct poolstate_controller_addr_t {
    bool            valid;  ///< True if controller info has been set.
    datalink_addr_t value;  ///< Learned controller address from broadcasts.
};

/// @brief System information (time-of-day and firmware version).
struct poolstate_system_t {
    poolstate_controller_addr_t addr;     ///< Learned controller address from broadcasts.
    poolstate_tod_t             tod;      ///< Current time-of-day on controller.
    poolstate_modes_t           modes;    ///< Controller mode flags.
    poolstate_version_t         version;  ///< Controller firmware version.
};

/// @}

/// @name Thermostat Structures
/// @brief Structures for pool/spa thermostat state.
/// @{

/// @brief Heat source setting with validity flag.
struct poolstate_heat_src_t {
    bool               valid;  ///< True if heat source has been set.
    network_heat_src_t value;  ///< Heat source type (off, heater, solar, etc.).
};

/// @brief Thermostat type enumeration.
enum class poolstate_thermo_typ_t : uint8_t {
    POOL = 0,  ///< Pool thermostat.
    SPA  = 1   ///< Spa thermostat.
};

/// @brief Thermostat state (temperature, setpoint, heat source, heating status).
struct poolstate_thermo_t {
    poolstate_uint8_t    temp_in_f;       ///< Current temperature in Fahrenheit (from ctrl_state_bcast, ctrl_heat_resp).
    poolstate_uint8_t    set_point_in_f;  ///< Target temperature in Fahrenheit (from ctrl_heat_resp, ctrl_heat_set).
    poolstate_heat_src_t heat_src;        ///< Heat source setting (from ctrl_state_bcast, ctrl_heat_resp, ctrl_heat_set).
    poolstate_bool_t     heating;         ///< True if actively heating (from ctrl_state_bcast).
};

/// @}

/// @name Schedule Structures
/// @brief Structures for circuit scheduling.
/// @{

/// @brief Schedule entry for a circuit.
struct poolstate_sched_t {
    bool     valid;   ///< True if schedule data is valid.
    bool     active;  ///< True if schedule is active/enabled.
    uint16_t start;   ///< Start time in minutes since midnight.
    uint16_t stop;    ///< Stop time in minutes since midnight.
};

/// @}

/// @name Temperature Structures
/// @brief Enumeration for temperature sensor types.
/// @{

/// @brief Temperature sensor type enumeration.
enum class poolstate_temp_typ_t : uint8_t {
    AIR   = 0,  ///< Air temperature sensor.
    WATER = 1   ///< Water temperature sensor.
};

/// @}

/// @name Circuit Structures
/// @brief Structures for circuit state.
/// @{

/// @brief Circuit state (active and delay flags).
struct poolstate_circuit_t {
    poolstate_bool_t active;  ///< True if circuit is currently active/on.
    poolstate_bool_t delay;   ///< True if circuit is in delay mode.
};

/// @}

/// @name Pump Structures
/// @brief Structures for variable-speed pump state.
/// @{

/// @brief Pump mode with validity flag.
struct poolstate_pump_mode_t {
    bool                    valid;  ///< True if mode has been set.
    network_pump_run_mode_t value;  ///< Pump mode (filter, manual, etc.).
};

/// @brief Pump state with validity flag.
struct poolstate_pump_state_t {
    bool                 valid;  ///< True if state has been set.
    network_pump_state_t value;  ///< Pump state (running, stopped, etc.).
};

/// @brief Complete pump status structure.
struct poolstate_pump_t {
    poolstate_time_t       time;     ///< Pump's internal clock time (from pump_status_resp).
    poolstate_pump_mode_t  mode;     ///< Operating mode (from pump_status_resp, pump_run_mode).
    poolstate_bool_t       running;  ///< True if pump is running (from pump_status_resp, pump_run).
    poolstate_pump_state_t state;    ///< Pump state (from pump_status_resp).
    poolstate_uint16_t     power;    ///< Power consumption in watts (from pump_status_resp).
    poolstate_uint16_t     flow;     ///< Flow rate in GPM (from pump_status_resp).
    poolstate_uint16_t     speed;    ///< Speed in RPM (from pump_status_resp).
    poolstate_uint16_t     level;    ///< Programmed level (from pump_status_resp).
    poolstate_uint8_t      error;    ///< Error code (from pump_status_resp).
    poolstate_time_t       timer;    ///< Remaining timer (from pump_status_resp).
};

/// @}

/// @name Chlorinator Structures
/// @brief Structures for salt chlorine generator state.
/// @{

/// @brief Chlorinator status/error flags.
enum class poolstate_chlor_status_typ_t : uint8_t {
    OTHER      = 0x00,  ///< Unknown or other status.
    LOW_FLOW   = 0x01,  ///< Low water flow detected.
    LOW_SALT   = 0x02,  ///< Salt level too low.
    HIGH_SALT  = 0x04,  ///< Salt level too high.
    UNKNOWN_08 = 0x08,  ///< Reserved/unknown flag.
    CLEAN_CELL = 0x10,  ///< Cell needs cleaning.
    UNKNOWN_20 = 0x20,  ///< Reserved/unknown flag.
    COLD       = 0x40,  ///< Water too cold for chlorination.
    OK         = 0x80   ///< Normal operation.
};

/// @brief Chlorinator status with validity flag.
struct poolstate_chlor_status_t {
    bool                         valid;  ///< True if status has been set.
    poolstate_chlor_status_typ_t value;  ///< Status/error code (from chlor_level_resp).
};

/// @brief Chlorinator name with validity flag.
struct poolstate_chlor_name_t {
    bool valid;                                           ///< True if name has been set.
    char value[sizeof(network_chlor_name_t) + 1];     ///< Null-terminated name string.
};

/// @brief Complete chlorinator state.
struct poolstate_chlor_t {
    poolstate_chlor_name_t   name;    ///< Chlorinator name (from chlor_name_resp).
    poolstate_uint8_t        level;   ///< Chlorination level percentage (from chlor_level_set).
    poolstate_uint16_t       salt;    ///< Salt level in PPM (from chlor_level_resp, chlor_name_resp).
    poolstate_chlor_status_t status;  ///< Status/error flags (from chlor_level_resp).
};

/// @}

/// @name Complete Pool State
/// @brief Main structure containing all pool system state.
/// @{

/**
 * @brief Complete pool state structure for the pool automation system.
 *
 * @details
 * This structure contains the full state of the pool system, including:
 * - System information (date, time, firmware version)
 * - Temperature readings (air, water)
 * - Thermostat states (pool, spa)
 * - Schedules for each circuit
 * - Mode bitsets
 * - Circuit states (active, delay)
 * - Pump status (mode, running, power, etc.)
 * - Chlorinator status (name, level, salt, status)
 */
struct poolstate_t {
    poolstate_system_t  system;                                          ///< System info (date, time, firmware).
    poolstate_chlor_t   chlor;                                           ///< Chlorinator status.
    poolstate_pump_t    pumps[enum_count<datalink_pump_id_t>()];         ///< Pump status array.
    poolstate_circuit_t circuits[enum_count<network_pool_circuit_t>()];  ///< Circuit states.
    poolstate_thermo_t  thermos[enum_count<poolstate_thermo_typ_t>()];   ///< Thermostat states.
    poolstate_uint8_t   temps[enum_count<poolstate_temp_typ_t>()];       ///< Temperature readings.
    poolstate_sched_t   scheds[enum_count<network_pool_circuit_t>()];    ///< Circuit schedules.
};

/// @}

/// @name Pool State Manager
/// @brief Class for tracking pool state changes.
/// @{

    // forward declaration of OpnPool class
class OpnPool;

/**
 * @brief Class for managing and tracking pool state changes.
 *
 * @details
 * Maintains a copy of the last known pool state and provides methods to
 * update, retrieve, and compare state for change detection.
 */
class PoolState {

  public:
    /// @brief Default constructor. Initializes state to zero/invalid.
    PoolState() {
        memset(&last_, 0, sizeof(poolstate_t));
    }

    /// @brief Destructor.
    ~PoolState() {}

    /**
     * @brief Stores a new pool state.
     *
     * @param[in] state Pointer to the new state to store.
     */
    void set(poolstate_t const * const state) {
        memcpy(&last_, state, sizeof(poolstate_t));
    }

    /**
     * @brief Retrieves the current stored pool state.
     *
     * @param[out] state Pointer to receive the current state.
     */
    void get(poolstate_t * const state) {
        memcpy(state, &last_, sizeof(poolstate_t));
    }

    /**
     * @brief Checks if a given state differs from the stored state.
     *
     * @param[in] state Pointer to the state to compare.
     * @return          True if the states differ, false if identical.
     */
    bool has_changed(poolstate_t const * const state) {
        return memcmp(&last_, state, sizeof(poolstate_t)) != 0;
    }

  private:
    poolstate_t last_ = {};  ///< Last known pool state (zero-initialized sets .valid to false).
};

/// @}

}  // namespace opnpool
}  // namespace esphome
