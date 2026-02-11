/**
 * @file poolstate_rx.cpp
 * @brief Implements pool state update logic for OPNpool ESPHome integration.
 *
 * @details
 * This file implements the core logic for updating and maintaining the pool system state
 * in the OPNpool component. The pool state update layer acts as the bridge between
 * low-level protocol/network messages and the high-level software model of the pool
 * controller and its peripherals (pump, chlorinator, circuits, sensors, etc.).
 *
 * Each supported message type is processed by a dedicated handler function, ensuring
 * modular, robust, and maintainable state updates. The updated state is used for
 * publishing sensor values, driving automation, and integrating with ESPHome and Home
 * Assistant entities.
 *
 * Verbose debug logging and diagnostics are supported via cJSON objects, allowing
 * detailed inspection of state changes and message processing for troubleshooting and
 * development.
 *
 * Design notes:
 * - ESPHome operates in a single-threaded environment, so explicit thread safety
 *   measures are not required within the pool_task context.
 * - Closely coupled with pool_state.h, network_msg.h, and poolstate_rx_log.h for data
 *   structures and logging.
 * - Intended as the main entry point for protocol-driven state updates in the OPNpool
 *   component.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright 2014, 2019, 2022, 2026, Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esphome/core/log.h>
#include <cJSON.h>
#include <type_traits>

#include "utils/to_str.h"
#include "utils/enum_helpers.h"
#include "pool_task/network.h"
#include "pool_task/network_msg.h"
#include "ipc/ipc.h"
#include "poolstate.h"
#include "opnpool.h"
#include "poolstate_rx_log.h"
#include "opnpool_ids.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

namespace esphome {
namespace opnpool {

namespace poolstate_rx {

constexpr char TAG[] = "poolstate_rx";

/// @name State Update Helpers
/// @brief Internal functions for updating pool state fields from message data.
/// @{

static void
_update_circuit_active_from_bits(poolstate_circuit_t * const arr, uint16_t const bits, uint8_t const count)
{
    for (uint16_t ii = 0, mask = 0x0001; ii < count; ++ii, mask <<= 1) {
        arr[ii].active = {
            .valid = true,
            .value = (bits & mask) != 0
        };
        ESP_LOGVV(TAG, "  arr[%u] = %u", ii, arr[ii].active.value);
    }
}

static void 
_update_circuit_delay_from_bits(poolstate_circuit_t * const arr, uint16_t const bits, uint8_t const count)
{
    for (uint16_t ii = 0, mask = 0x0001; ii < count; ++ii, mask <<= 1) {
        arr[ii].delay = {
            .valid = true,
            .value = (bits & mask) != 0
        };
        ESP_LOGVV(TAG, "  arr[%u] = %u", ii, arr[ii].delay.value);
    }
}

static void
_update_circuits(cJSON * const dbg, network_ctrl_state_bcast_t const * const msg, poolstate_circuit_t * const circuits)
{
    constexpr uint8_t pool_idx = enum_index(network_pool_circuit_t::POOL);
    constexpr uint8_t spa_idx  = enum_index(network_pool_circuit_t::SPA);

        // update circuits[].active
    uint16_t const bitmask_active_circuits = msg->active.to_uint16();
    _update_circuit_active_from_bits(circuits, bitmask_active_circuits, enum_count<network_pool_circuit_t>());

        // if both SPA and POOL bits are set, only SPA runs
    if (circuits[spa_idx].active.value) {
        circuits[pool_idx].active.value = false;
    }

        // update circuits[].delay
    uint8_t const bitmask_delay_circuits = msg->delay;
    _update_circuit_delay_from_bits(circuits, bitmask_delay_circuits, enum_count<network_pool_circuit_t>());

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_circuits(dbg, poolstate_rx_log::KEY_CIRCUITS, circuits);
    }
}

static void
_update_thermos(cJSON * const dbg, network_ctrl_state_bcast_t const * const msg, poolstate_thermo_t * const thermos, poolstate_circuit_t const * const circuits)
{
        // update circuits.thermos (only update when the pump is running)
    constexpr uint8_t pool_therm_idx = enum_index(poolstate_thermo_typ_t::POOL);
    constexpr uint8_t spa_therm_idx  = enum_index(poolstate_thermo_typ_t::SPA);
    static_assert(pool_therm_idx < enum_count<poolstate_thermo_typ_t>(), "pool_therm_idx OOB");
    static_assert(spa_therm_idx < enum_count<poolstate_thermo_typ_t>(), "spa_therm_idx OOB");
    poolstate_thermo_t * const pool_thermo = &thermos[pool_therm_idx];
    poolstate_thermo_t * const spa_thermo  = &thermos[spa_therm_idx];

    constexpr uint8_t pool_circuit_idx = enum_index(network_pool_circuit_t::POOL);
    constexpr uint8_t spa_circuit_idx  = enum_index(network_pool_circuit_t::SPA);
    poolstate_bool_t const * const pool_circuit = &circuits[pool_circuit_idx].active;
    poolstate_bool_t const * const spa_circuit  = &circuits[spa_circuit_idx].active;

    if (pool_circuit->valid && pool_circuit->value) {
        pool_thermo->temp_in_f = {
            .valid = true,
            .value = msg->pool_temp
        };
    }
    if (spa_circuit->valid && spa_circuit->value) {
        spa_thermo->temp_in_f = {
            .valid = true,
            .value = msg->spa_temp
        };
    }
    pool_thermo->heating = {
        .valid = true,
        .value = msg->heat_status.get_pool()
    };
    pool_thermo->heat_src = {
        .valid = true,
        .value = msg->heat_src.get_pool()
    };
    spa_thermo->heating  = {
        .valid = true,
        .value = msg->heat_status.get_spa()
    };
    spa_thermo->heat_src = {
        .valid = true,
        .value = msg->heat_src.get_spa()
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_thermos(dbg, poolstate_rx_log::KEY_THERMOS, thermos, true, true, true);
    }    
}

static void
_update_system_modes(cJSON * const dbg, network_ctrl_state_bcast_t const * const msg, poolstate_modes_t * const mode)
{
    *mode = {
        .valid = true,
        .value = msg->modes
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_mode(dbg, poolstate_rx_log::KEY_MODES, *mode);
    }
}

static void
_update_system_time(cJSON * const dbg, network_ctrl_state_bcast_t const * const msg, poolstate_time_t * const time)
{
    *time = {
        .valid = true,
        .value = msg->time
    };
    // PS date is updated through `network_ctrl_time`

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_time(dbg, poolstate_rx_log::KEY_TIME, time);
    }
}

static void
_update_temps(cJSON * const dbg, network_ctrl_state_bcast_t const * const msg, poolstate_uint8_t * const temps)
{
    uint8_t const air_idx = enum_index(poolstate_temp_typ_t::AIR);
    uint8_t const water_idx = enum_index(poolstate_temp_typ_t::WATER);
    static_assert(air_idx < enum_count<poolstate_temp_typ_t>(), "size err for air_idx");
    static_assert(water_idx < enum_count<poolstate_temp_typ_t>(), "size err for water_idx");

    temps[air_idx] = {
        .valid = true,
        .value = msg->solar_temp_1 // 2BD should probably be air_temp on other systems
    };
    temps[water_idx] = {
        .valid = true,
        .value = msg->pool_temp
    };

    ESP_LOGVV(TAG, "Air %u, Spa %u, Water %u Solar1 %u, Solar2 %u", msg->air_temp, msg->spa_temp, msg->pool_temp, msg->solar_temp_1, msg->solar_temp_2);

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_temps(dbg, poolstate_rx_log::KEY_TEMPS, temps);
    }
}

/// @}

/// @name Pump Message Handlers
/// @brief Handlers for messages from variable-speed pumps.
/// @{

/**
 * @brief            Process a pump register set message and log the register update.
 *
 * @param dbg        Optional JSON object for verbose debug logging.
 * @param pump_id  The device ID of the pump.
 * @param msg        Pointer to the received network_pump_reg_set_t message.
 *
 * This function decodes the pump register address and value from the message and, if
 * verbose logging is enabled, logs the register update to the debug JSON object.
 */
static void
_pump_reg_set(cJSON * const dbg, network_pump_reg_set_t const * const msg, datalink_pump_id_t const pump_id)
{
    if (!msg) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    // no change to poolstate

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_pump_reg_set(dbg, poolstate_rx_log::KEY_REG, pump_id, msg);
    }
}


/**
 * @brief            Process a pump register set response message and log the register value.
 *
 * @param dbg        Optional JSON object for verbose debug logging.
 * @param pump_id  The device ID of the pump.
 * @param msg        Pointer to the received network_pump_reg_resp_t message.
 *
 * This function decodes the register value from the message and, if verbose logging is
 * enabled, logs the value to the debug JSON object.
 */
static void
_pump_reg_resp(cJSON * const dbg, network_pump_reg_resp_t const * const msg, datalink_pump_id_t const pump_id)
{
    if (!msg) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    // no change to poolstate

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_pump_reg_resp(dbg, poolstate_rx_log::KEY_RESP, pump_id, msg);
    }
}

/**
 * @brief            Process a pump control set/resp message and log the control value.
 *
 * @param dbg        Optional JSON object for verbose debug logging.
 * @param pump_id  The device ID of the pump.
 * @param msg        The network_pump_ctrl_t message received.
 *
 * This function logs the pump control value to the debug JSON object if verbose logging
 * is enabled.
 */
static void
_pump_ctrl(cJSON * const dbg, network_pump_ctrl_t const msg, datalink_pump_id_t const pump_id)
{
    // no change to poolstate

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
       poolstate_rx_log::add_pump_ctrl(dbg, poolstate_rx_log::KEY_CTRL, pump_id, msg);
    }
}

/**
 * @brief            Process a pump mode set/resp message, update the pool state, and log the mode.
 *
 * @param dbg        Optional JSON object for verbose debug logging.
 * @param pump_id  The device ID of the pump.
 * @param msg        Pointer to the received network_pump_mode_*_t message.
 * @param pumps      Pointer to the array of poolstate_pump_set_t structures to update.
 *
 * This function updates the pump mode in the pool state and logs the mode to the debug
 * JSON object if verbose logging is enabled.
 */
static void
_pump_mode(cJSON * const dbg, network_pump_run_mode_t const msg, datalink_pump_id_t const pump_id, poolstate_pump_t * const pumps)
{
    if (!pumps) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    auto pump = &pumps[enum_index(pump_id)];

    pump->mode = {
        .valid = true,
        .value = msg
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_pump_mode(dbg, poolstate_rx_log::KEY_MODE, pump_id, pump->mode.value);
    }
}

/**
 * @brief            Process a pump run set/resp message, update the running state, and log the status.
 *
 * @param dbg        Optional JSON object for verbose debug logging.
 * @param pump_id  The device ID of the pump.
 * @param msg        Pointer to the received network_pump_run_*_t message.
 * @param pumps      Pointer to the array of poolstate_pump_set_t structures to update.
 *
 * This function updates the running state of the pump in the pool state and logs the
 * status to the debug JSON object if verbose logging is enabled.
 */
static void
_pump_running(cJSON * const dbg, network_pump_running_t const * const msg, datalink_pump_id_t const pump_id, poolstate_pump_t * const pumps)
{
    if (!msg || !pumps) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    auto pump = &pumps[enum_index(pump_id)];

    bool const running     = msg->is_on();
    bool const not_running = msg->is_off();
    if (!running && !not_running) {
        ESP_LOGW(TAG, "running state err 0x%02X in %s", msg->raw, __func__);
        return;
    }    

    pump->running = {
        .valid = true,
        .value = running
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {   
        poolstate_rx_log::add_pump_running(dbg, poolstate_rx_log::KEY_RUNNING, pump_id, pump->running.value);
    }
}

/**
 * @brief            Process a pump status response message, update the pool state, and log the status.
 *
 * @param dbg        Optional JSON object for verbose debug logging.
 * @param msg        Pointer to the received network_pump_status_resp_t message.
 * @param pump_id  The device ID of the pump.
 * @param pumps      Pointer to the array of poolstate_pump_t structures to update.
 *
 * This function updates all pump status fields in the pool state and logs the status to
 * the debug JSON object if verbose logging is enabled.
 */
static void
_pump_status(cJSON * const dbg, network_pump_status_resp_t const * const msg, datalink_pump_id_t const pump_id, poolstate_pump_t * const pumps)
{
    if (!msg || !pumps) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    auto pump = &pumps[enum_index(pump_id)];

    bool const running     = msg->running.is_on();
    bool const not_running = msg->running.is_off();

    if (!running && !not_running) {
        ESP_LOGW(TAG, "running state err 0x%02X (%u %u) in %s", msg->running.raw, running, not_running, __func__);
        return;
    }

    *pump = {
        .time = {
            .valid  = true,
            .value  = msg->clock,
        },
        .mode = {
            .valid = true,
            .value = msg->mode
        },
        .running = {
            .valid = true,
            .value = running
        },
        .state = {
            .valid = true,
            .value = msg->state
        },
        .power = {
            .valid = true,
            .value = msg->power.to_uint16()
        },
        .flow = {
            .valid = true,
            .value = msg->flow
        },
        .speed = {
            .valid = true,
            .value = msg->speed.to_uint16()
        },
        .level = {
            .valid = true,      
            .value = msg->level
        },
        .error = {
            .valid = true,
            .value = msg->error
        },
        .timer = {
            .valid = true,
            .value = msg->remaining
        }
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_pump(dbg, poolstate_rx_log::KEY_STATUS, pump_id, pump);
    }
}

/// @}

/// @name Controller Message Handlers
/// @brief Handlers for messages from the pool controller.
/// @{

/**
 * @brief       Process a controller time message and update the pool state.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_ctrl_time_t message.
 * @param state Pointer to the poolstate_t structure to update.
 * 
 * This function updates the system time and date in the pool state based on the
 * received controller time message. If verbose logging is enabled, the updated
 * time-of-day is added to the debug JSON object.
 */
static void
_ctrl_time(cJSON * const dbg, network_ctrl_time_t const * const msg, poolstate_t * const state)
{
    if (!msg || !state) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    state->system.tod = {
        .date = {
            .valid = true,
            .value = msg->date
        },
        .time = {
            .valid = true,
            .value = msg->time
        }
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_time_and_date(dbg, poolstate_rx_log::KEY_TOD, &state->system.tod);
    }
}

/**
 * @brief       Process a controller heat response message and update the pool state.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_ctrl_heat_resp_t message.
 * @param state Pointer to the poolstate_t structure to update.
 *
 * This function updates the pool and spa thermostat values (temperature, set point, heat
 * source) in the pool state based on the received controller heat response message. If
 * verbose logging is enabled, the updated thermostat information is added to the debug
 * JSON object.
 */
static void
_ctrl_heat_resp(cJSON * const dbg, network_ctrl_heat_resp_t const * const msg, poolstate_t * const state)
{
    if (!msg || !state) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    uint8_t const pool_idx = enum_index(poolstate_thermo_typ_t::POOL);
    uint8_t const  spa_idx = enum_index(poolstate_thermo_typ_t::SPA);
    static_assert(pool_idx < enum_count<poolstate_thermo_typ_t>(), "size mismatch for pool_idx");
    static_assert( spa_idx < enum_count<poolstate_thermo_typ_t>(), "size mismatch for spa_idx");

    poolstate_thermo_t * const pool_thermo = &state->thermos[pool_idx];
    poolstate_thermo_t * const spa_thermo  = &state->thermos[spa_idx];

    pool_thermo->temp_in_f =  {
        .valid = true,
        .value = msg->pool_temp
    };
    pool_thermo->set_point_in_f =  {
        .valid = true,
        .value = msg->pool_set_point
    };
    pool_thermo->heat_src = {
        .valid = true,
        .value = msg->heat_src.get_pool()
    };
    spa_thermo->temp_in_f =  {
        .valid = true,
        .value = msg->spa_temp
    };
    spa_thermo->set_point_in_f =  {
        .valid = true,
        .value = msg->spa_set_point
    };
    spa_thermo->heat_src = {
        .valid = true,
        .value = msg->heat_src.get_spa()
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_thermos(dbg, poolstate_rx_log::KEY_THERMOS, state->thermos, true, true, false);
    }
}

/**
 * @brief       Process a controller heat set message and update the pool state.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_ctrl_heat_set_t message.
 * @param state Pointer to the poolstate_t structure to update.
 *
 * This function updates the set point and heat source for the pool and spa thermostats
 * in the pool state based on the received controller heat set message. If verbose logging
 * is enabled, the updated thermostat information is added to the debug JSON object.
 */
static void
_ctrl_heat_set(cJSON * const dbg, network_ctrl_heat_set_t const * const msg, poolstate_t * const state)
{
    if (!msg || !state) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }
    uint8_t const pool_idx = enum_index(poolstate_thermo_typ_t::POOL);
    uint8_t const spa_idx  = enum_index(poolstate_thermo_typ_t::SPA);
    static_assert(pool_idx < enum_count<poolstate_thermo_typ_t>(), "size mismatch for pool_idx");
    static_assert( spa_idx < enum_count<poolstate_thermo_typ_t>(), "size mismatch for spa_idx");

    poolstate_thermo_t * const pool_thermo = &state->thermos[pool_idx];
    poolstate_thermo_t * const spa_thermo  = &state->thermos[spa_idx];
    
    pool_thermo->set_point_in_f = {
        .valid = true,
        .value = msg->pool_set_point
    };
    pool_thermo->heat_src = {
        .valid = true,
        .value = msg->heat_src.get_pool()
    };
    spa_thermo->set_point_in_f = {
        .valid = true,
        .value = msg->spa_set_point
    };
    spa_thermo->heat_src = {
        .valid = true,
        .value = msg->heat_src.get_spa()
    };
    
    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_thermos(dbg, poolstate_rx_log::KEY_THERMOS, state->thermos, false, true, false);
    }
}

/**
 * @brief             Optionally log raw hex bytes from a controller message.
 *
 * @param dbg         Optional JSON object for verbose debug logging.
 * @param bytes       Pointer to the array of bytes received from the controller.
 * @param no_of_bytes Number of bytes in the array.
 *
 * This function logs the received bytes in hexadecimal format to the debug JSON object
 * if verbose logging is enabled. It does not modify the pool state.
 */
static void
_ctrl_hex_bytes(cJSON * const dbg, uint8_t const * const bytes, uint8_t no_of_bytes)
{
    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        cJSON * const array = cJSON_CreateArray();
        
        for (uint_least8_t ii = 0; ii < no_of_bytes; ii++) {
            char hex_str[3];  // "XX\0"
            ESP_LOGVV(TAG, "byte[%u] = 0x%02X", ii, bytes[ii]);
            snprintf(hex_str, sizeof(hex_str), "%02X", bytes[ii]);
            cJSON_AddItemToArray(array, cJSON_CreateString(hex_str));
        }
        cJSON_AddItemToObject(dbg, "raw", array);
    }
}

/**
 * @brief       Process a controller circuit set message and update the pool state.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_ctrl_circuit_set_t message.
 * @param state Pointer to the poolstate_t structure to update.
 *
 * This function updates the active state of a specific pool circuit based on the
 * received controller circuit set message. If verbose logging is enabled, the updated
 * circuit value is added to the debug JSON object.
 */
static void
_ctrl_circuit_set(cJSON * const dbg, network_ctrl_circuit_set_t const * const msg, poolstate_t * const state)
{
    if (!msg || !state) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }
    if (msg->circuit_plus_1 == 0) {
        ESP_LOGW(TAG, "circuit_plus_1 == 0");
        return;
    }

    uint8_t const circuit_idx = msg->circuit_plus_1 - 1;

    if (circuit_idx >= enum_count<network_pool_circuit_t>()) {
        ESP_LOGW(TAG, "circuit %u>=%u", circuit_idx, enum_count<network_pool_circuit_t>());
        return;
    }

    state->circuits[circuit_idx].active = {
        .valid = true,
        .value = msg->get_value()
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        network_pool_circuit_t const circuit = static_cast<network_pool_circuit_t>(circuit_idx);

        cJSON_AddBoolToObject(dbg, enum_str(circuit), msg->get_value());
    }
}

/**
 * @brief       Process a controller schedule response message and update the pool state.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_ctrl_sched_resp_t message.
 * @param state Pointer to the poolstate_t structure to update.
 *
 * This function updates the pool state with schedule information for each circuit based on
 * the received controller schedule response message. If verbose logging is enabled, the
 * updated schedule information is added to the debug JSON object.
 */
static void
_ctrl_sched_resp(cJSON * const dbg, network_ctrl_sched_resp_t const * const msg, poolstate_t * const state)
{
    if (!msg || !state) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }
    poolstate_sched_t * const state_scheds = state->scheds;
    memset(state_scheds, 0, sizeof(state->scheds));

    for (const auto& sched : msg->scheds) {

        if (sched.circuit_plus_1 == 0) {
            // nothing wrong with this. the schedule entry is just unused
            continue;
        }

        network_pool_circuit_t const circuit =
            static_cast<network_pool_circuit_t>(sched.circuit_plus_1 - 1);
        uint8_t const circuit_idx = enum_index(circuit);

        uint16_t const start = sched.prg_start.to_uint16();
        uint16_t const stop  = sched.prg_stop.to_uint16();

        if (circuit_idx < std::size(state->scheds)) {
            state_scheds[circuit_idx] = {
                .valid = true,
                .active = true,
                .start = start,
                .stop = stop
            };
        } else {
            ESP_LOGW(TAG, "circuit %u>=%zu", circuit_idx, std::size(state->scheds));
        }
    }

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_scheds(dbg, poolstate_rx_log::KEY_SCHEDS, state->scheds);
    }
}

/**
 * @brief       Process a controller state broadcast message and update the pool state.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_ctrl_state_bcast_t message.
 * @param state Pointer to the poolstate_t structure to update.
 *
 * This function updates the entire pool state (circuits, modes, thermostats, system time,
 * and temperatures) based on the received controller state broadcast message. If verbose
 * logging is enabled, the updated state is added to the debug JSON object.
 */
static void
_ctrl_state(cJSON * const dbg, network_ctrl_state_bcast_t const * const msg,  poolstate_t * state)
{
    if (!msg || !state) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    _update_temps(dbg, msg, state->temps);
    _update_thermos(dbg, msg, state->thermos, state->circuits); 
    _update_system_modes(dbg, msg, &state->system.modes);
    _update_system_time(dbg, msg, &state->system.tod.time);
    _update_circuits(dbg, msg, state->circuits);

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_state(dbg, poolstate_rx_log::KEY_STATE, state);
    }
}

/**
 * @brief       Process a controller version response message and update the pool state.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_ctrl_version_resp_t message.
 * @param state Pointer to the poolstate_t structure to update.
 *
 * This function updates the firmware version in the pool state based on the received
 * controller version response message. If verbose logging is enabled, the updated
 * version information is added to the debug JSON object.
 */
static void
_ctrl_version_resp(cJSON * const dbg, network_ctrl_version_resp_t const * const msg, poolstate_t * const state)
{
    if (!msg || !state) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    state->system.version = {
        .valid = true,
        .major = msg->major,
        .minor = msg->minor
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_version(dbg, poolstate_rx_log::KEY_FIRMWARE, &state->system.version);
    }
}

/**
 * @brief       Process a controller set acknowledgment message and log the result.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_ctrl_set_ack_t message.
 *
 * This function logs the acknowledgment type to the debug JSON object if verbose logging
 * is enabled.
 */
static void
_ctrl_set_ack(cJSON * const dbg, network_ctrl_set_ack_t const * const msg)
{
    if (!msg) {
        ESP_LOGW(TAG, "null to %s", __func__);
        return;
    }

    // no change to poolstate
    //     2BD we could.., e.g. when a circuit is changed ..

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        cJSON_AddStringToObject(dbg, poolstate_rx_log::KEY_ACK, enum_str(msg->typ));
    }
}

/// @}

/// @name Chlorinator Message Handlers
/// @brief Handlers for messages from the salt chlorine generator.
/// @{

    // returns the first matching status flag, or OTHER if none match
constexpr poolstate_chlor_status_typ_t
_get_chlor_status_from_error(uint8_t const error)
{
    using T = poolstate_chlor_status_typ_t;

    if (error & static_cast<uint8_t>(T::LOW_FLOW))   return T::LOW_FLOW;
    if (error & static_cast<uint8_t>(T::LOW_SALT))   return T::LOW_SALT;
    if (error & static_cast<uint8_t>(T::HIGH_SALT))  return T::HIGH_SALT;
    if (error & static_cast<uint8_t>(T::CLEAN_CELL)) return T::CLEAN_CELL;
    if (error & static_cast<uint8_t>(T::COLD))       return T::COLD;
    if (error & static_cast<uint8_t>(T::OK))         return T::OK;
    return T::OTHER;
}

static void
_chlor_control_req(cJSON * const dbg, network_chlor_control_req_t const * const msg)
{
    if (!msg) { ESP_LOGW(TAG, "null to %s", __func__); return; }

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        if (msg->is_control_req()) {
            cJSON_AddStringToObject(dbg, poolstate_rx_log::KEY_SUBCMD, "CONTROL_REQ");
        } else {
            cJSON_AddNumberToObject(dbg, poolstate_rx_log::KEY_SUBCMD, msg->sub_cmd);
        }
    }
}

static void
_chlor_model_req(cJSON * const dbg, network_chlor_model_req_t const * const msg)
{
    if (!msg) { ESP_LOGW(TAG, "null to %s", __func__); return; }

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        if (msg->is_get_typ()) {
            cJSON_AddStringToObject(dbg, poolstate_rx_log::KEY_SUBCMD, "MODEL_REQ");
        } else {
            cJSON_AddNumberToObject(dbg, poolstate_rx_log::KEY_SUBCMD, msg->typ);
        }
    }
}

/**
 * @brief       Process a chlorine generator name response message, update the pool state, and log the status.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_chlor_model_resp_t message.
 * @param chlor Pointer to the poolstate_chlor_t structure to update.
 *
 * This function updates the chlorine generator name and salt level in the pool state and
 * logs the status to the debug JSON object if verbose logging is enabled.
 */
static void
_chlor_model_resp(cJSON * const dbg, network_chlor_model_resp_t const * const msg, poolstate_chlor_t * const chlor)
{
    if (!msg || !chlor) { ESP_LOGW(TAG, "null to %s", __func__); return; }

    chlor->salt = {
        .valid = true,
        .value = static_cast<uint16_t>(msg->salt * 50)
    };

    uint32_t name_size = sizeof(chlor->name.value);
    strncpy(chlor->name.value, msg->name, name_size);
    chlor->name.value[name_size - 1] = '\0';
    chlor->name.valid = true;

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        cJSON_AddNumberToObject(dbg, poolstate_rx_log::KEY_SALT, chlor->salt.value);
        cJSON_AddStringToObject(dbg, poolstate_rx_log::KEY_NAME, chlor->name.value);
        ESP_LOGV(TAG, "Chlorine status updated: salt=%u, name=%s", chlor->salt.value, chlor->name.value);
    }
}

/**
 * @brief       Process a chlorine generator level set message, update the pool state, and log the status.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_chlor_level_set_t message.
 * @param chlor Pointer to the poolstate_chlor_t structure to update.
 *
 * This function updates the chlorine generator level in the pool state and logs the
 * status to the debug JSON object if verbose logging is enabled.
 */
static void
_chlor_level_set(cJSON * const dbg, network_chlor_level_set_t const * const msg, poolstate_chlor_t * const chlor)
{
    if (!msg || !chlor) { ESP_LOGW(TAG, "null to %s", __func__); return; }   

    chlor->level = {
        .valid = true,
        .value = msg->level
    };

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        cJSON_AddNumberToObject(dbg, poolstate_rx_log::KEY_LEVEL, chlor->level.value);
    }
}

/**
 * @brief       Process a chlorine generator level set response message, update the pool state, and log the status.
 *
 * @param dbg   Optional JSON object for verbose debug logging.
 * @param msg   Pointer to the received network_chlor_level_resp_t message.
 * @param chlor Pointer to the poolstate_chlor_t structure to update.
 *
 * This function updates the chlorine generator salt level and status in the pool state and logs the status to
 * the debug JSON object if verbose logging is enabled.
 * Note: good salt range is 2600 to 4500 ppm.
 */
static void
_chlor_level_set_resp(cJSON * const dbg, network_chlor_level_resp_t const * const msg, poolstate_chlor_t * const chlor)
{
    if (!msg || !chlor) { ESP_LOGW(TAG, "null to %s", __func__); return; }

    chlor->salt = {
        .valid = true,
        .value = static_cast<uint16_t>(msg->salt * 50)
    };
    chlor->status = {
        .valid = true,
        .value = _get_chlor_status_from_error(msg->error)
    };  

    if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
        poolstate_rx_log::add_chlor_resp(dbg, poolstate_rx_log::KEY_CHLOR, chlor);
    }
}

/// @}

/// @name Main Entry Point
/// @brief Primary function for dispatching network messages to handlers.
/// @{

/**
 * @brief           Process a received network message and update the pool state.
 *
 * @param msg       Pointer to the received network_msg_t message (must not be null).
 * @param new_state Pointer to the poolstate_t structure to update (must not be null).
 * @return          ESP_OK if the state was updated and processed successfully, ESP_FAIL otherwise.
 *
 * This function dispatches the received network message to the appropriate handler,
 * updating the provided pool state structure based on the message contents. It also
 * logs detailed debug information to a cJSON object if verbose logging is enabled.
 *
 * The function sets the 'valid' flag in the state, updates all relevant fields according
 * to the message type, and optionally logs the update as JSON. It is the main entry point
 * for state updates in response to protocol messages.
 */
esp_err_t
update_state(network_msg_t const * const msg, poolstate_t * const new_state)
{
    if (msg == nullptr || new_state == nullptr) { ESP_LOGW(TAG, "null to %s", __func__); return ESP_FAIL; }

    bool const verbose = ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE;
    bool const very_verbose = ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE;

        // adjust the new_state based on the incoming message
    cJSON * const dbg = verbose ? cJSON_CreateObject() : nullptr; 

    bool const is_to_pump = msg->dst.is_pump();
    datalink_pump_id_t const pump_id = is_to_pump ? msg->dst.get_pump_id() : msg->src.get_pump_id();

    switch (msg->typ) {
        case network_msg_typ_t::IGNORE:
            break;
        case network_msg_typ_t::PUMP_REG_SET:
            _pump_reg_set(dbg, &msg->u.a5.pump_reg_set, pump_id);
            break;
        case network_msg_typ_t::PUMP_REG_RESP:
            _pump_reg_resp(dbg, &msg->u.a5.pump_reg_resp, pump_id);
            break;
        case network_msg_typ_t::PUMP_REMOTE_CTRL_SET:
        case network_msg_typ_t::PUMP_REMOTE_CTRL_RESP:
            _pump_ctrl(dbg, msg->u.a5.pump_ctrl, pump_id);
            break;
        case network_msg_typ_t::PUMP_RUN_MODE_SET:
        case network_msg_typ_t::PUMP_RUN_MODE_RESP:
            _pump_mode(dbg, msg->u.a5.pump_mode, pump_id, new_state->pumps);
            break;
        case network_msg_typ_t::PUMP_RUN_SET:
        case network_msg_typ_t::PUMP_RUN_RESP:
            _pump_running(dbg, &msg->u.a5.pump_running, pump_id, new_state->pumps);
            break;
        case network_msg_typ_t::PUMP_STATUS_REQ:
             break;
        case network_msg_typ_t::PUMP_STATUS_RESP:
            _pump_status(dbg, &msg->u.a5.pump_status_resp, pump_id, new_state->pumps);
            break;
        case network_msg_typ_t::CTRL_SET_ACK:  // response to various set requests
            _ctrl_set_ack(dbg, &msg->u.a5.ctrl_set_ack);
            break;
        case network_msg_typ_t::CTRL_CIRCUIT_SET:
            _ctrl_circuit_set(dbg, &msg->u.a5.ctrl_circuit_set, new_state);
            break;
        case network_msg_typ_t::CTRL_SCHED_REQ:
            break;
        case network_msg_typ_t::CTRL_SCHED_RESP:
            _ctrl_sched_resp(dbg, &msg->u.a5.ctrl_sched_resp, new_state);
            break;
        case network_msg_typ_t::CTRL_STATE_BCAST:
            _ctrl_state(dbg, &msg->u.a5.ctrl_state_bcast, new_state);
            break;
        case network_msg_typ_t::CTRL_TIME_REQ:
            break;
        case network_msg_typ_t::CTRL_TIME_SET:
        case network_msg_typ_t::CTRL_TIME_RESP:
            _ctrl_time(dbg, &msg->u.a5.ctrl_time, new_state);
            break;
        case network_msg_typ_t::CTRL_HEAT_REQ:
            break;
        case network_msg_typ_t::CTRL_HEAT_RESP:
            _ctrl_heat_resp(dbg, &msg->u.a5.ctrl_heat_resp, new_state);
            break;
        case network_msg_typ_t::CTRL_HEAT_SET:
            _ctrl_heat_set(dbg, &msg->u.a5.ctrl_heat_set, new_state);
            break;
        case network_msg_typ_t::CTRL_LAYOUT_REQ:
        case network_msg_typ_t::CTRL_LAYOUT_RESP:
        case network_msg_typ_t::CTRL_LAYOUT_SET:
            break;
        case network_msg_typ_t::CTRL_VALVE_REQ:
            break;
        case network_msg_typ_t::CTRL_VALVE_RESP:
            _ctrl_hex_bytes(dbg, msg->u.raw, sizeof(network_ctrl_valve_resp_t));
            break;
        case network_msg_typ_t::CTRL_VERSION_REQ:
            break;
        case network_msg_typ_t::CTRL_VERSION_RESP:
            _ctrl_version_resp(dbg, &msg->u.a5.ctrl_version_resp, new_state);
            break;
        case network_msg_typ_t::CTRL_SOLARPUMP_REQ:
            break;
        case network_msg_typ_t::CTRL_SOLARPUMP_RESP:
            _ctrl_hex_bytes(dbg, msg->u.raw, sizeof(network_ctrl_solarpump_resp_t));
            break;
        case network_msg_typ_t::CTRL_DELAY_REQ:
            break;
        case network_msg_typ_t::CTRL_DELAY_RESP:
            _ctrl_hex_bytes(dbg, msg->u.raw, sizeof(network_ctrl_delay_resp_t));
            break;
        case network_msg_typ_t::CTRL_HEAT_SETPT_REQ:
            break;
        case network_msg_typ_t::CTRL_HEAT_SETPT_RESP:
            _ctrl_hex_bytes(dbg, msg->u.raw, sizeof(network_ctrl_heat_setpt_resp_t));
            break;
        case network_msg_typ_t::CTRL_CIRC_NAMES_REQ:
            break;
        case network_msg_typ_t::CTRL_CIRC_NAMES_RESP:
            _ctrl_hex_bytes(dbg, msg->u.raw, sizeof(network_ctrl_circ_names_resp_t));
            break;
        case network_msg_typ_t::CTRL_SCHEDS_REQ:
            break;
        case network_msg_typ_t::CTRL_SCHEDS_RESP:
            _ctrl_hex_bytes(dbg, msg->u.raw, sizeof(network_ctrl_scheds_resp_t));
            break;
        case network_msg_typ_t::CTRL_CHEM_REQ:
            break;
        case network_msg_typ_t::CHLOR_CONTROL_REQ:
            _chlor_control_req(dbg, &msg->u.ic.chlor_control_req);
            break;
        case network_msg_typ_t::CHLOR_CONTROL_RESP:
            _ctrl_hex_bytes(dbg, msg->u.raw, sizeof(network_chlor_control_resp_t));
            break;
        case network_msg_typ_t::CHLOR_MODEL_REQ:
            _chlor_model_req(dbg, &msg->u.ic.chlor_model_req);
            break;
        case network_msg_typ_t::CHLOR_MODEL_RESP:
            _chlor_model_resp(dbg, &msg->u.ic.chlor_model_resp, &new_state->chlor);
            break;
        case network_msg_typ_t::CHLOR_LEVEL_SET:
            _chlor_level_set(dbg, &msg->u.ic.chlor_level_set, &new_state->chlor);
            break;
        case network_msg_typ_t::CHLOR_LEVEL_RESP:
            _chlor_level_set_resp(dbg, &msg->u.ic.chlor_level_resp, &new_state->chlor);
            break;
        default:
            ESP_LOGW(TAG, "Received unknown message type: %u", static_cast<uint8_t>(msg->typ));
            break;
    }

    bool const frequent = //msg->typ == network_msg_typ_t::CTRL_STATE_BCAST      ||
                          msg->typ == network_msg_typ_t::IGNORE                ||   
                          msg->typ == network_msg_typ_t::CHLOR_LEVEL_SET       ||
                          msg->typ == network_msg_typ_t::PUMP_REMOTE_CTRL_SET  ||
                          msg->typ == network_msg_typ_t::PUMP_REMOTE_CTRL_RESP ||
                          msg->typ == network_msg_typ_t::PUMP_RUN_SET          ||
                          msg->typ == network_msg_typ_t::PUMP_RUN_RESP         ||
                          msg->typ == network_msg_typ_t::PUMP_STATUS_REQ       ||
                          msg->typ == network_msg_typ_t::PUMP_STATUS_RESP;

    if ((verbose && !frequent) || very_verbose) {
        size_t const json_size = 1500;
        char * const json = static_cast<char *>(calloc(1, json_size));
        if (json == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate memory for JSON string");
            cJSON_Delete(dbg);
            return ESP_FAIL;
        }
        bool print_success = cJSON_PrintPreallocated(dbg, json, json_size, false);
        if (!print_success) {
            ESP_LOGE(TAG, "Failed to print JSON string");  // increase size?
            free(json);
            cJSON_Delete(dbg);
            return ESP_FAIL;
        }
        ESP_LOGV(TAG, "{%s: %s}\n", enum_str(msg->typ), json);
        free(json);
    }
    cJSON_Delete(dbg);
    return ESP_OK;
}

/// @}

}  // namespace poolstate_rx

} // namespace opnpool
} // namespace esphome  