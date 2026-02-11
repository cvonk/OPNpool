/**
 * @file poolstate_rx_log.cpp
 * @brief PoolState: log state as a JSON-formatted string
 *
 * @details
 * This file provides functions to serialize the OPNpool controller's internal state and
 * its subcomponents (system, pump, chlorinator, thermostats, schedules, etc.) into a
 * compact JSON representation for logging.  Each function adds a specific part of the
 * pool state to a cJSON object, using type-safe enum-to-string helpers and value checks
 * to ensure clarity and correctness in the output.
 *
 * These functions are kept separate from poolstate_rx.cpp because their purpose is
 * to provide logging functionality, and separating them helps to avoid making that file
 * too large.
 *
 * ESPHome operates in a single-threaded environment, so explicit thread safety measures
 * are not required within the pool_task context.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/log.h>
#include <cJSON.h>
#include <string.h>
#include <cstddef>

#include "utils/to_str.h"
#include "utils/enum_helpers.h"
#include "pool_task/network.h"
#include "poolstate.h"
#include "poolstate_rx_log.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

namespace poolstate_rx {
namespace poolstate_rx_log {

/**
 * @brief Creates or returns a JSON object for the given key.
 *
 * @param[in] obj The parent JSON object.
 * @param[in] key The key for the new child object, or nullptr to return parent.
 * @return        The created child object, or parent if key is nullptr.
 */
[[nodiscard]] static cJSON *
_create_item(cJSON * const obj, char const * const key)
{
    if (key == nullptr) {
        return obj;
    }
    cJSON * const item = cJSON_CreateObject();

    cJSON_AddItemToObject(obj, key, item);
    return item;
}

/**
 * @brief Adds system information (time, date, firmware) to a JSON object.
 *
 * @param[in] obj    The parent JSON object.
 * @param[in] key    The key under which to add the system object.
 * @param[in] system Pointer to the poolstate_system_t structure.
 */
void
_add_system(cJSON * const obj, char const * const key, poolstate_system_t const * const system)
{
    cJSON * const item = _create_item(obj, key);

    add_time_and_date(item, KEY_TOD, &system->tod);
    add_version(item, KEY_FIRMWARE, &system->version);
}

/**
 * @brief Adds active circuit states to a JSON object.
 *
 * @param[in] obj     The parent JSON object.
 * @param[in] key     The key under which to add the active circuits object.
 * @param[in] circuit Pointer to the first poolstate_circuit_t in the array.
 */
static void
_add_circuit_active(cJSON * const obj, char const * const key, poolstate_circuit_t const * circuit)
{
    cJSON * const item = _create_item(obj, key);

    for (auto typ : magic_enum::enum_values<network_pool_circuit_t>()) {
        cJSON_AddBoolToObject(item, enum_str(typ), circuit->active.value);
        circuit++;
    }
}

/**
 * @brief Adds delay circuit states to a JSON object.
 *
 * @param[in] obj     The parent JSON object.
 * @param[in] key     The key under which to add the delay circuits object.
 * @param[in] circuit Pointer to the first poolstate_circuit_t in the array.
 */
static void
_add_circuit_delay(cJSON * const obj, char const * const key, poolstate_circuit_t const * circuit)
{
    cJSON * const item = _create_item(obj, key);

    for (auto typ : magic_enum::enum_values<network_pool_circuit_t>()) {
        cJSON_AddBoolToObject(item, enum_str(typ), circuit->delay.value);
        circuit++;
    }
}

/**
 * @brief Adds pump mode as a string to a JSON object.
 *
 * @param[in] obj  The parent JSON object.
 * @param[in] key  The key for the pump mode value.
 * @param[in] mode The pump mode value.
 */
static void
_add_pump_mode(cJSON * const obj, char const * const key, network_pump_run_mode_t const mode)
{
    cJSON_AddStringToObject(obj, key, mode.to_str());
}

/**
 * @brief Adds pump running status to a JSON object.
 *
 * @param[in] obj     The parent JSON object.
 * @param[in] key     The key for the running status.
 * @param[in] running The pump running status (true if running).
 */
static void
_add_pump_running(cJSON * const obj, char const * const key, bool const running)
{
    cJSON_AddBoolToObject(obj, key, running);
}

/**
 * @brief Adds circuit information (active and delay states) to a JSON object.
 *
 * @param[in] obj      The parent JSON object.
 * @param[in] key      The key under which to add the circuits object.
 * @param[in] circuits Pointer to the array of poolstate_circuit_t structures.
 */
void
add_circuits(cJSON * const obj, char const * const key, poolstate_circuit_t const * const circuits)
{
    cJSON * const item = _create_item(obj, key);

    _add_circuit_active(item, KEY_ACTIVE, circuits);
    _add_circuit_delay(item, KEY_DELAY, circuits);
}

/**
 * @brief Adds mode information to a JSON object.
 *
 * @param[in] obj  The parent JSON object.
 * @param[in] key  The key under which to add the modes object.
 * @param[in] mode The poolstate_modes_t structure.
 */
void
add_mode(cJSON * const obj, char const * const key, poolstate_modes_t const mode)
{
    cJSON * const item = _create_item(obj, key);

    cJSON_AddBoolToObject(item, "service", mode.value.is_service_mode());
    cJSON_AddBoolToObject(item, "temp_inc", mode.value.is_temp_increase_mode());
    cJSON_AddBoolToObject(item, "freeze_prot", mode.value.is_freeze_protection_mode());
    cJSON_AddBoolToObject(item, "timeout", mode.value.is_timeout_mode());
}

/**
 * @brief Adds temperature information to a JSON object.
 *
 * @param[in] obj   The parent JSON object.
 * @param[in] key   The key under which to add the temperatures object.
 * @param[in] temps Pointer to the array of poolstate_uint8_t temperature values.
 */
void
add_temps(cJSON * const obj, char const * const key, poolstate_uint8_t const * temps)
{
    cJSON * const item = _create_item(obj, key);

    poolstate_uint8_t const * temp = temps;
    for (auto typ : magic_enum::enum_values<poolstate_temp_typ_t>()) {
        if (temp->value != 0xFF && temp->value != 0x00) {
            cJSON_AddNumberToObject(item, enum_str(typ), temp->value);
        }
        temp++;
    }
}

/**
 * @brief Adds time information to a JSON object for logging.
 *
 * @param[in] obj  The parent JSON object.
 * @param[in] key  The key under which to add the time object.
 * @param[in] time Pointer to the poolstate_time_t structure containing the time.
 */
void
add_time(cJSON * const obj, char const * const key, poolstate_time_t const * const time)
{
    cJSON * const item = _create_item(obj, key);

    cJSON_AddStringToObject(item, KEY_TIME, time_str(time->value.hour, time->value.minute));
}

/**
 * @brief Adds time and date information to a JSON object for logging.
 *
 * @param[in] obj The parent JSON object.
 * @param[in] key The key under which to add the time and date object.
 * @param[in] tod Pointer to the poolstate_tod_t structure containing the time and date.
 */
void
add_time_and_date(cJSON * const obj, char const * const key, poolstate_tod_t const * const tod)
{
    cJSON * const item = _create_item(obj, key);

    cJSON_AddStringToObject(item, KEY_TIME, time_str(tod->time.value.hour, tod->time.value.minute));
    cJSON_AddStringToObject(item, KEY_DATE, date_str(tod->date.value.year, tod->date.value.month, tod->date.value.day));
}

/**
 * @brief Adds version information to a JSON object for logging.
 *
 * @param[in] obj     The parent JSON object.
 * @param[in] key     The key under which to add the version string.
 * @param[in] version Pointer to the poolstate_version_t structure.
 */
void
add_version(cJSON * const obj, char const * const key, poolstate_version_t const * const version)
{
    cJSON_AddStringToObject(obj, key, version_str(version->major, version->minor));
}

/**
 * @brief Adds thermostat information to a JSON object for logging.
 *
 * @param[in] obj         The parent JSON object.
 * @param[in] key         The key under which to add the thermostat array.
 * @param[in] thermos     Pointer to the array of poolstate_thermo_t structures.
 * @param[in] showTemp    Whether to include temperature values.
 * @param[in] showSp      Whether to include set point values.
 * @param[in] showHeating Whether to include heating status.
 */
void
add_thermos(cJSON * const obj, char const * const key, poolstate_thermo_t const * thermos,
            bool const showTemp, bool showSp, bool const showHeating)
{
    cJSON * const item = _create_item(obj, key);

    for (auto typ : magic_enum::enum_values<poolstate_thermo_typ_t>()) {

        cJSON * const sub_item = _create_item(item, enum_str(typ));

        if (showTemp) {
            cJSON_AddNumberToObject(sub_item, KEY_TEMP, thermos->temp_in_f.value);
        }

        if (showSp) {
            cJSON_AddNumberToObject(sub_item, KEY_SP, thermos->set_point_in_f.value);
        }
        cJSON_AddStringToObject(sub_item, KEY_SRC, enum_str(thermos->heat_src.value));

        if (showHeating) {
            cJSON_AddBoolToObject(sub_item, KEY_HEATING, thermos->heating.value);
        }
        thermos++;
    }
}

/**
 * @brief Adds schedule information to a JSON object for logging.
 *
 * @param[in] obj   The parent JSON object.
 * @param[in] key   The key under which to add the schedule array.
 * @param[in] sched Pointer to the first poolstate_sched_t structure in the array.
 */
void
add_scheds(cJSON * const obj, char const * const key, poolstate_sched_t const * sched)
{
    cJSON * const item = _create_item(obj, key);

    for (auto circuit : magic_enum::enum_values<network_pool_circuit_t>()) {
        if (sched->active) {
            cJSON * const sub_item = _create_item(item, enum_str(circuit));

            network_time_t const start_time = {
                .hour   = static_cast<uint8_t>(sched->start / 60),
                .minute = static_cast<uint8_t>(sched->start % 60)
            };
            network_time_t const stop_time = {
                .hour   = static_cast<uint8_t>(sched->stop / 60),
                .minute = static_cast<uint8_t>(sched->stop % 60)
            };
            cJSON_AddStringToObject(sub_item, KEY_START, time_str(start_time.hour, start_time.minute));
            cJSON_AddStringToObject(sub_item, KEY_STOP,  time_str( stop_time.hour,  stop_time.minute));
        }
        sched++;
    }
}

/**
 * @brief Adds the controller pool state to a JSON object for logging.
 *
 * @param[in] obj   The parent JSON object.
 * @param[in] key   The key under which to add the pool state object.
 * @param[in] state Pointer to the poolstate_t structure to log.
 */
void
add_state(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON * const item = _create_item(obj, key);

    add_thermos(item, KEY_THERMOS, state->thermos, true, false, true);
    add_scheds(item, KEY_SCHEDS, state->scheds);
    add_mode(item, KEY_MODES, state->system.modes);
    add_temps(item, KEY_TEMPS, state->temps);
    add_circuits(item, KEY_CIRCUITS, state->circuits);
    _add_system(item, KEY_SYSTEM, &state->system);
}

/**
 * @brief Adds pump information to a JSON object for logging.
 *
 * @param[in] obj     The parent JSON object.
 * @param[in] key     The key under which to add the pump object.
 * @param[in] pump_id The device ID for the pump.
 * @param[in] pump    Pointer to the poolstate_pump_t structure to log.
 */
void
add_pump(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, poolstate_pump_t const * const pump)
{
    cJSON * const item = _create_item(obj, key);

    _add_pump_mode(item, KEY_MODE, pump->mode.value);
    _add_pump_running(item, KEY_RUNNING, pump->running.value);

    cJSON_AddStringToObject(item, KEY_TIME, time_str(pump->time.value.hour, pump->time.value.minute));
    cJSON_AddStringToObject(item, KEY_STATE, enum_str(pump->state.value));
    cJSON_AddStringToObject(item, KEY_ID, enum_str(pump_id));
    cJSON_AddNumberToObject(item, KEY_POWER, pump->power.value);
    cJSON_AddNumberToObject(item, KEY_SPEED, pump->speed.value);
    if (pump->flow.value) {
        cJSON_AddNumberToObject(item, KEY_FLOW, pump->flow.value);
    }
    if (pump->level.value) {
        cJSON_AddNumberToObject(item, KEY_LEVEL, pump->level.value);
    }
    cJSON_AddNumberToObject(item, KEY_ERROR, pump->error.value);
    cJSON_AddStringToObject(item, KEY_TIME, time_str(pump->timer.value.hour, pump->timer.value.minute));
}

/**
 * @brief Adds pump program value to a JSON object for logging.
 *
 * @param[in] obj     The parent JSON object.
 * @param[in] key     The key under which to add the pump program value.
 * @param[in] pump_id The device ID for the pump.
 * @param[in] value   The pump program value to log.
 */
void
add_pump_reg_set(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, network_pump_reg_set_t const * const reg)
{
    cJSON * const item = _create_item(obj, key);

    cJSON_AddStringToObject(item, KEY_ID, enum_str(pump_id));
    cJSON_AddStringToObject(item, KEY_ADDRESS, enum_str(reg->address));
    cJSON_AddStringToObject(item, KEY_OPERATION, reg->operation.to_str());

    if (reg->operation.is_write()) {
        cJSON_AddNumberToObject(item, KEY_VALUE, reg->value.to_uint16());
    }
}

void
add_pump_reg_resp(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, network_pump_reg_resp_t const * const reg)
{
    cJSON * const item = _create_item(obj, key);

    cJSON_AddStringToObject(item, KEY_ID, enum_str(pump_id));
    cJSON_AddNumberToObject(item, KEY_VALUE, reg->value.to_uint16());
}

/**
 * @brief Adds pump control mode to a JSON object for logging.
 *
 * @param[in] obj     The parent JSON object.
 * @param[in] key     The key under which to add the pump control value.
 * @param[in] pump_id The device ID for the pump.
 * @param[in] ctrl    The pump control value to log.
 */
void
add_pump_ctrl(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, network_pump_ctrl_t const ctrl)
{
    cJSON * const item = _create_item(obj, enum_str(pump_id));

    cJSON_AddBoolToObject(item, key, ctrl.is_local());
}

/**
 * @brief Adds pump mode to a JSON object for logging.
 *
 * @param[in] obj     The parent JSON object.
 * @param[in] key     The key under which to add the pump mode value.
 * @param[in] pump_id The device ID for the pump.
 * @param[in] mode    The pump mode value to log.
 */
void
add_pump_mode(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, network_pump_run_mode_t const mode)
{
    cJSON * const item = _create_item(obj, enum_str(pump_id));

    _add_pump_mode(item, key, mode);
}

/**
 * @brief Adds pump running status to a JSON object for logging.
 *
 * @param[in] obj     The parent JSON object.
 * @param[in] key     The key under which to add the running status.
 * @param[in] pump_id The device ID for the pump.
 * @param[in] running The pump running status to log (true if running).
 */
void
add_pump_running(cJSON * const obj, char const * const key, datalink_pump_id_t const pump_id, bool const running)
{
    cJSON * const item = _create_item(obj, enum_str(pump_id));

    _add_pump_running(item, key, running);
}

/**
 * @brief Adds chlorinator response information to a JSON object for logging.
 *
 * @param[in] obj   The parent JSON object.
 * @param[in] key   The key under which to add the chlorinator response object.
 * @param[in] chlor Pointer to the poolstate_chlor_t structure to log.
 */
void
add_chlor_resp(cJSON * const obj, char const * const key, poolstate_chlor_t const * const chlor)
{
    cJSON * const item = _create_item(obj, key);

    cJSON_AddNumberToObject(item, KEY_SALT, chlor->salt.value);
    cJSON_AddStringToObject(item, KEY_STATUS, enum_str(chlor->status.value));
}

}  // namespace poolstate_rx_log
}  // namespace poolstate_rx

}  // namespace opnpool
}  // namespace esphome
