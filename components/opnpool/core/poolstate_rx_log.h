/**
 * @file poolstate_rx_log.h
 * @brief JSON logging functions for pool state serialization.
 *
 * @details
 * Declares functions to serialize pool controller state and its subcomponents
 * (system info, pump, chlorinator, thermostats, schedules, etc.) into JSON
 * format for logging purposes. Uses cJSON library for JSON object construction.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#ifndef __cplusplus
# error "Requires C++ compilation"
#endif

#include <esp_system.h>
#include <esp_types.h>
#include <cJSON.h>

#if defined(MAGIC_ENUM_RANGE_MIN)
# undef MAGIC_ENUM_RANGE_MIN
#endif
#if defined(MAGIC_ENUM_RANGE_MAX)
# undef MAGIC_ENUM_RANGE_MAX
#endif
#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include "utils/magic_enum.h"

namespace esphome {
namespace opnpool {

/// @name Forward Declarations
/// @brief Avoids circular dependencies with poolstate types.
/// @{
struct poolstate_t;
struct poolstate_tod_t;
struct poolstate_time_t;
struct poolstate_version_t;
struct poolstate_thermo_t;
struct poolstate_sched_t;
struct poolstate_chlor_t;
struct poolstate_bool_t;
struct poolstate_uint8_t;
struct poolstate_circuit_t;
struct poolstate_pump_t;
struct network_pump_ctrl_t;
struct network_pump_run_mode_t;
enum class datalink_pump_id_t : uint8_t;
/// @}

namespace poolstate_rx {
namespace poolstate_rx_log {

/// @name JSON Key Constants
/// @brief String constants used as keys in JSON output.
/// @{
constexpr char const * const KEY_TIME      = "time";      ///< Time field key.
constexpr char const * const KEY_DATE      = "date";      ///< Date field key.
constexpr char const * const KEY_FIRMWARE  = "firmware";  ///< Firmware version key.
constexpr char const * const KEY_TOD       = "tod";       ///< Time-of-day key.
constexpr char const * const KEY_TEMP      = "temp";      ///< Temperature key.
constexpr char const * const KEY_SP        = "sp";        ///< Set point key.
constexpr char const * const KEY_SRC       = "src";       ///< Heat source key.
constexpr char const * const KEY_HEATING   = "heating";   ///< Heating status key.
constexpr char const * const KEY_START     = "start";     ///< Schedule start key.
constexpr char const * const KEY_STOP      = "stop";      ///< Schedule stop key.
constexpr char const * const KEY_ACTIVE    = "active";    ///< Active circuit key.
constexpr char const * const KEY_DELAY     = "delay";     ///< Delay circuit key.
constexpr char const * const KEY_SYSTEM    = "system";    ///< System info key.
constexpr char const * const KEY_TEMPS     = "temps";     ///< Temperatures key.
constexpr char const * const KEY_THERMOS   = "thermos";   ///< Thermostats key.
constexpr char const * const KEY_PUMP      = "pump";      ///< Pump info key.
constexpr char const * const KEY_CHLOR     = "chlor";     ///< Chlorinator key.
constexpr char const * const KEY_CIRCUITS  = "circuits";  ///< Circuits key.
constexpr char const * const KEY_SCHEDS    = "scheds";    ///< Schedules key.
constexpr char const * const KEY_MODES     = "modes";     ///< Modes key.
constexpr char const * const KEY_NAME      = "name";      ///< Name field key.
constexpr char const * const KEY_LEVEL     = "level";     ///< Level field key.
constexpr char const * const KEY_SALT      = "salt";      ///< Salt level key.
constexpr char const * const KEY_STATUS    = "status";    ///< Status field key.
constexpr char const * const KEY_MODE      = "mode";      ///< Mode field key.
constexpr char const * const KEY_RUNNING   = "running";   ///< Running status key.
constexpr char const * const KEY_STATE     = "state";     ///< State field key.
constexpr char const * const KEY_POWER     = "power";     ///< Power field key.
constexpr char const * const KEY_SPEED     = "speed";     ///< Speed field key.
constexpr char const * const KEY_FLOW      = "flow";      ///< Flow field key.
constexpr char const * const KEY_ERROR     = "error";     ///< Error field key.
constexpr char const * const KEY_TIMER     = "timer";     ///< Timer field key.
constexpr char const * const KEY_RESP      = "resp";      ///< Response key.
constexpr char const * const KEY_CTRL      = "local_ctrl";///< Local control key.
constexpr char const * const KEY_SUBCMD    = "sub_cmd";   ///< Sub-command key.
constexpr char const * const KEY_ACK       = "ack";       ///< Acknowledgment key.
constexpr char const * const KEY_ID        = "id";        ///< Pump Device ID key.
constexpr char const * const KEY_REG       = "reg";       ///< Pump register key.
constexpr char const * const KEY_ADDRESS   = "address";   ///< Pump register address key.
constexpr char const * const KEY_OPERATION = "operation"; ///< Pump register operation key.
constexpr char const * const KEY_VALUE     = "value";     ///< Pump register value key.


/// @}

/// @name Pool State Logging Functions
/// @brief Functions to add pool state components to JSON objects.
/// @{
void add_time(cJSON * const obj, char const * const key, poolstate_time_t const * const time);
void add_time_and_date(cJSON * const obj, char const * const key, poolstate_tod_t const * const tod);
void add_version(cJSON * const obj, char const * const key, poolstate_version_t const * const version);
void add_thermos(cJSON * const obj, char const * const key, poolstate_thermo_t const * thermos, bool const showTemp, bool const showSp, bool const showHeating);
void add_scheds(cJSON * const obj, char const * const key, poolstate_sched_t const * scheds);
void add_mode(cJSON * const obj, char const * const key, poolstate_modes_t const mode);
void add_temps(cJSON * const obj, char const * const key, poolstate_uint8_t const * temps);
void add_circuits(cJSON * const obj, char const * const key, poolstate_circuit_t const * const circuits);
void add_state(cJSON * const obj, char const * const key, poolstate_t const * const state);
/// @}

/// @name Pump Logging Functions
/// @brief Functions to add pump-specific data to JSON objects.
/// @{
void add_pump_reg_set(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, network_pump_reg_set_t const * const reg);
void add_pump_reg_resp(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, network_pump_reg_resp_t const * const reg);
void add_pump_ctrl(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, network_pump_ctrl_t const ctrl);
void add_pump_mode(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, network_pump_run_mode_t const mode);
void add_pump_running(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, bool const running);
void add_pump(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, poolstate_pump_t const * const pumps);
/// @}

/// @name Chlorinator Logging Functions
/// @brief Functions to add chlorinator data to JSON objects.
/// @{
void add_chlor_resp(cJSON * const obj, char const * const key, poolstate_chlor_t const * const chlor);
/// @}

}  // namespace poolstate_rx_log
}  // namespace poolstate_rx

}  // namespace opnpool
}  // namespace esphome
