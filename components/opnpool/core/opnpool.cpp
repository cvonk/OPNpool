/**
 * @file opnpool.cpp
 * @brief Implementation of the OPNpool component for ESPHome.
 *
 * @details
 * This file implements the OPNpool component, providing seamless, bidirectional
 * integration between the pool controller and the ESPHome ecosystem. It follows a
 * publish/subscribe model:
 *   - Publishes changes in PoolState to ESPHome climate, switch, sensor, binary sensor,
 *     and text sensor entities, ensuring Home Assistant always reflects the latest pool
 *     state and equipment status.
 *   - Subscribes to ESPHome entity state changes, enacting requests to set switches and
 *     climate controls on the physical pool equipment.
 *   - Spawns and supervises the pool_task (FreeRTOS), which manages low-level RS485
 *     communication, protocol parsing, and network message processing for robust hardware
 *     integration.
 *
 * The design emphasizes modularity, extensibility, and maintainability, leveraging helper
 * functions for protocol abstraction and FreeRTOS primitives for robust task scheduling
 * and inter-task communication. Extensive logging and debug output are provided for
 * diagnostics, troubleshooting, and protocol analysis.
 *
 * ESPHome operates in a single-threaded environment, so explicit thread safety measures
 * are not required within the main task context. 
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <string>
#include <cctype>
#include <cstdlib>
#include <esphome/core/log.h>
#include <esphome/core/hal.h>

#include "poolstate_rx.h"
#include "pool_task/skb.h"
#include "pool_task/rs485.h"
#include "pool_task/datalink.h"
#include "pool_task/datalink_pkt.h"
#include "ipc/ipc.h"
#include "pool_task/pool_task.h"
#include "opnpool.h"
#include "utils/to_str.h"
#include "poolstate.h"
#include "entities/opnpool_climate.h"
#include "entities/opnpool_switch.h"
#include "entities/opnpool_sensor.h"
#include "entities/opnpool_binary_sensor.h"
#include "entities/opnpool_text_sensor.h"
#include "entities/opnpool_number.h"
#include "entities/opnpool_select.h"
#include "opnpool_ids.h"
#include "pool_task/network.h"
#include "pool_task/network_msg.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {
  
constexpr char TAG[] = "opnpool";

constexpr uint32_t    POOL_TASK_STACK_SIZE = 2 * 4096;
constexpr UBaseType_t TO_POOL_QUEUE_LEN = 6;
constexpr UBaseType_t TO_MAIN_QUEUE_LEN = 10;

/**
 * @brief            Calls dump_config() on an entity if it exists.
 *
 * @tparam EntityT   The entity type (climate, switch, sensor, etc.).
 * @param[in] entity Pointer to the entity, or nullptr.
 */
template<typename EntityT>
static void
_dump_if(EntityT * const entity)
{
    if (entity != nullptr) {
        entity->dump_config();
    }
}

/**
 * @brief            Publishes a value to an entity if it exists and the value is valid.
 *
 * @tparam EntityT   The entity type.
 * @tparam ValueT    The value wrapper type (must have .valid and .value members).
 * @param[in] entity Pointer to the entity, or nullptr.
 * @param[in] base   The value wrapper containing validity flag and value.
 */
template<typename EntityT, typename ValueT>
static void
_publish_if(EntityT * const entity, ValueT const base)
{
    if (entity != nullptr && base.valid) {
        entity->publish_value_if_changed(base.value);
    }
}

/**
 * @brief            Publishes an enum value as a string to an entity if it exists and is valid.
 *
 * @tparam EntityT   The entity type.
 * @tparam ValueT    The value wrapper type (must have .valid and .value members).
 * @param[in] entity Pointer to the entity, or nullptr.
 * @param[in] base   The value wrapper containing validity flag and enum value.
 */
template<typename EntityT, typename ValueT>
static void
_publish_enum_if(EntityT * const entity, ValueT const base)
{
    if (entity != nullptr && base.valid) {
        entity->publish_value_if_changed(enum_str(base.value));
    }
}

/**
 * @brief            Publishes a string value as a string to an entity if it exists and is valid.
 *
 * @tparam EntityT   The entity type.
 * @tparam ValueT    The value wrapper type (must have .valid and .value members).
 * @param[in] entity Pointer to the entity, or nullptr.
 * @param[in] base   The value wrapper containing validity flag and string value.
 */
template<typename EntityT, typename ValueT>
static void
_publish_str_if(EntityT * const entity, ValueT const base)
{
    if (entity != nullptr && base.valid) {
        entity->publish_value_if_changed(base.value.to_str());
    }
}

/**
 * @brief            Publishes a schedule as a formatted time range string.
 *
 * @param[in] sensor Pointer to the text sensor entity, or nullptr.
 * @param[in] sched  Pointer to the schedule data structure.
 */
static void
_publish_schedule_if(OpnPoolTextSensor * const sensor, poolstate_sched_t const * const sched)
{
    if (sensor == nullptr || !sched->valid) {
        return;
    }
    char buf[16];  // HH:MM-HH:MM\0

    snprintf(buf, sizeof(buf), "%02d:%02d-%02d:%02d",
             sched->start / 60, sched->start % 60,
             sched->stop / 60, sched->stop % 60);

    sensor->publish_value_if_changed(buf);
}

/**
 * @brief            Publishes date and time as a formatted string.
 *
 * @param[in] sensor Pointer to the text sensor entity, or nullptr.
 * @param[in] tod    Pointer to the time-of-day data structure.
 */
static void
_publish_date_and_time_if(OpnPoolTextSensor * const sensor, poolstate_tod_t const * const tod)
{
    if (sensor == nullptr || tod == nullptr || !tod->time.valid) {
        return;
    }
    static char time_str[22];  // 2026-01-15 22:43\0

    if (tod->date.valid) {
        snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d",
                 2000 +tod->date.value.year, tod->date.value.month, tod->date.value.day,
                 tod->time.value.hour, tod->time.value.minute);
    } else {
        snprintf(time_str, sizeof(time_str), "%02d:%02d",
                 tod->time.value.hour, tod->time.value.minute);
    }
    sensor->publish_value_if_changed(time_str);
}

/**
 * @brief             Publishes firmware version as a formatted string.
 *
 * @param[in] sensor  Pointer to the text sensor entity, or nullptr.
 * @param[in] version Pointer to the version data structure.
 */
static void
_publish_version_if(OpnPoolTextSensor * const sensor, poolstate_system_t const * const system)
{
    if (sensor != nullptr && system != nullptr && system->version.valid) {
        static char fw_str[24];  // "Easy/Sun Touch 99.999\0"
        snprintf(fw_str, sizeof(fw_str), "Easy/Sun Touch %d.%03d", system->version.major, system->version.minor);
        sensor->publish_value_if_changed(fw_str);
    }
}

/**
 * @brief Publishes the four mode binary sensors if present and valid.
 *
 * @param[in] sensors Array of binary sensor pointers.
 * @param[in] modes   The modes value wrapper (must have .valid and .value members).
 */
static void
_publish_modes_if(OpnPoolBinarySensor * const * const  sensors, poolstate_modes_t const modes)
{
    if (!modes.valid) return;
    if (sensors[enum_index(binary_sensor_id_t::MODE_SERVICE)]) {
        sensors[enum_index(binary_sensor_id_t::MODE_SERVICE)]->publish_value_if_changed(modes.value.is_service_mode());
    }
    if (sensors[enum_index(binary_sensor_id_t::MODE_TEMPERATURE_INC)]) {
        sensors[enum_index(binary_sensor_id_t::MODE_TEMPERATURE_INC)]->publish_value_if_changed(modes.value.is_temp_increase_mode());
    }
    if (sensors[enum_index(binary_sensor_id_t::MODE_FREEZE_PROTECTION)]) {
        sensors[enum_index(binary_sensor_id_t::MODE_FREEZE_PROTECTION)]->publish_value_if_changed(modes.value.is_freeze_protection_mode());
    }
    if (sensors[enum_index(binary_sensor_id_t::MODE_TIMEOUT)]) {
        sensors[enum_index(binary_sensor_id_t::MODE_TIMEOUT)]->publish_value_if_changed(modes.value.is_timeout_mode());
    }
}

/**
 * @brief                Converts a thermostat type to its corresponding pool circuit index.
 *
 * @param[in] thermo_typ The thermostat type (POOL or SPA).
 * @return               The circuit index for the corresponding pool circuit.
 */
static uint8_t
_thermo_typ_to_pool_circuit_idx(poolstate_thermo_typ_t const thermo_typ)
{
    switch (thermo_typ) {
        case poolstate_thermo_typ_t::POOL:
            return enum_index(network_pool_circuit_t::POOL);
        case poolstate_thermo_typ_t::SPA:
            return enum_index(network_pool_circuit_t::SPA);
        default:
            ESP_LOGE(TAG, "Invalid thermo_typ: %u", static_cast<unsigned int>(thermo_typ));
            return 0;  // incorrect, but at least will prevent OOB array access if used
    }
}

/**
 * @brief  Set up the OpnPool component.
 *
 * This function initializes the OpnPool state, sets up inter-process communication (IPC)
 * queues, and starts the pool task that handles RS485 communication, datalink layer, and
 * network layer. It also publishes the interface firmware version if available.
 */
void 
OpnPool::setup() {
    
    ESP_LOGI(TAG, "Setting up OpnPool...");

        // instantiate Poolstate
    poolState_ = new PoolState();
    if (!poolState_) {
        ESP_LOGE(TAG, "Failed to instantiate PoolState");
        return;
    }

        // alloc IPC struct
    ipc_ = new ipc_t{};
    if (ipc_ == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate IPC struct");
        delete poolState_;
        return;
    }

        // assign in IPC struct
    ipc_->config.rs485_pins = rs485_pins_;
    ipc_->to_pool_q = xQueueCreate(TO_POOL_QUEUE_LEN, sizeof(network_msg_t));
    ipc_->to_main_q = xQueueCreate(TO_MAIN_QUEUE_LEN, sizeof(network_msg_t));
    if (!ipc_->to_main_q || !ipc_->to_pool_q) {
        ESP_LOGE(TAG, "Failed to create IPC queue(s)");
        if (ipc_->to_main_q) vQueueDelete(ipc_->to_main_q);
        if (ipc_->to_pool_q) vQueueDelete(ipc_->to_pool_q);
        delete ipc_;
        delete poolState_;
        return;
    }

        // spin off a pool_task to handle RS485 communication, datalink layer and network layer
    if (xTaskCreate(&pool_task, "pool_task", POOL_TASK_STACK_SIZE, this->ipc_, 3, &pool_task_handle_) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create pool_task");
        if (ipc_->to_main_q) vQueueDelete(ipc_->to_main_q);
        if (ipc_->to_pool_q) vQueueDelete(ipc_->to_pool_q);
        delete ipc_;
        delete poolState_;
        return;
    }

        // publish our firmware version
    auto * const interface_firmware = this->text_sensors_[static_cast<uint8_t>(text_sensor_id_t::INTERFACE_FIRMWARE)];
    if (interface_firmware != nullptr) {
#ifdef GIT_HASH
            interface_firmware->publish_state(GIT_HASH);
#else
            interface_firmware->publish_state("unknown");
#endif
    }

#ifdef USE_API_CUSTOM_SERVICES
        // expose Home Assistant services for schedule export/import
        // (requires 'custom_services: true' in the api: section)
    register_service(&OpnPool::on_export_schedules, "export_schedules");
    register_service(&OpnPool::on_import_schedule, "import_schedule", {"yaml"});
#endif

#ifdef USE_MATTER
        // Initialize Matter bridge if configured
    if (matter_config_.discriminator != 0 || matter_config_.passcode != 0) {
        matter_bridge_ = new matter::MatterBridge();
        if (matter_bridge_ == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate Matter bridge");
        } else {
            esp_err_t err = matter_bridge_->init(&matter_config_);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to initialize Matter bridge: %s", esp_err_to_name(err));
                delete matter_bridge_;
                matter_bridge_ = nullptr;
            } else {
                ESP_LOGI(TAG, "Matter bridge initialized successfully");

                // Log QR code for commissioning
                char qr_buf[64];
                if (matter_bridge_->get_qr_code(qr_buf, sizeof(qr_buf)) > 0) {
                    ESP_LOGI(TAG, "Matter QR code: %s", qr_buf);
                }
            }
        }
    }
#endif
}

/**
 * @brief Destructor for OpnPool component.
 *
 * @details
 * Cleans up resources including the pool task, IPC queues, and pool state.
 */
OpnPool::~OpnPool()
{
#ifdef USE_MATTER
    if (matter_bridge_ != nullptr) {
        delete matter_bridge_;
        matter_bridge_ = nullptr;
    }
#endif

    if (pool_task_handle_) {
        vTaskDelete(pool_task_handle_);
    }
    if (ipc_ != nullptr) {
        if (ipc_->to_main_q) vQueueDelete(ipc_->to_main_q);
        if (ipc_->to_pool_q) vQueueDelete(ipc_->to_pool_q);
        delete ipc_;
    }
    delete poolState_;
}


/**
 * @brief Main loop for the OpnPool component.
 *
 * This function is called repeatedly by the main ESPHome loop. It handles service
 * requests from the pool by checking the IPC queue for messages from the pool task.
 * Warning: don't do any blocking operations here.
 */
void
OpnPool::loop() {

    network_msg_t msg = {};

    if (xQueueReceive(ipc_->to_main_q, &msg, 0) == pdPASS) {  // check if a message is available

            // reset global string buffer (as a new cycle begins)
        name_reset_idx();

            // start with new_state being the current state
        poolstate_t new_state;
        poolState_->get(&new_state);

        if (msg.src.is_controller()) {
            new_state.system.addr = {
                .valid = true,
                .value = msg.src
            };
            ESP_LOGV(TAG, "learned controller address: 0x%02X", msg.src.addr);
        }

        if (poolstate_rx::update_state(&msg, &new_state) == ESP_OK) {

            if (poolState_->has_changed(&new_state)) {

                poolState_->set(&new_state);

                    // publish this as an update to the HA sensors 
                this->update_climates(&new_state);
                this->update_switches(&new_state);
                this->update_text_sensors(&new_state);
                this->update_analog_sensors(&new_state);
                this->update_binary_sensors(&new_state);
                this->update_numbers(&new_state);
            }
 
            ESP_LOGVV(TAG, "FYI Poolstate changed");

#ifdef USE_MATTER
                // Update Matter endpoints with new state
            if (matter_bridge_ != nullptr) {
                matter_bridge_->update_from_poolstate(&new_state);
            }
#endif
        }
    }

#ifdef USE_MATTER
        // Process pending Matter commands (from Matter controller → pool)
    if (matter_bridge_ != nullptr) {
        network_msg_t matter_cmd = {};
        while (matter_bridge_->get_pending_command(&matter_cmd)) {
            ESP_LOGD(TAG, "Processing Matter command: %s", enum_str(matter_cmd.typ));
            if (xQueueSend(ipc_->to_pool_q, &matter_cmd, 0) != pdPASS) {
                ESP_LOGW(TAG, "Failed to queue Matter command to pool_task");
            }
        }
    }
#endif
}

/**
 * @brief Dump the configuration of the OpnPool component.
 *
 * This function logs the configuration of the OpnPool component, including the RS485 pin
 * assignments and the configuration of all associated climate, switch, sensor, binary
 * sensor, and text sensor components.
 */
void 
OpnPool::dump_config() {

    ESP_LOGCONFIG(TAG, "OpnPool:");
    ESP_LOGCONFIG(TAG, "  RS485 rx pin: %u", this->ipc_->config.rs485_pins.rx_pin);
    ESP_LOGCONFIG(TAG, "  RS485 tx pin: %u", this->ipc_->config.rs485_pins.tx_pin);
    ESP_LOGCONFIG(TAG, "  RS485 rts pin: %u", this->ipc_->config.rs485_pins.rts_pin);

    for (auto idx : magic_enum::enum_values<climate_id_t>()) {
        _dump_if(this->climates_[enum_index(idx)]);
    }   
    for (auto idx : magic_enum::enum_values<switch_id_t>()) {
        _dump_if(this->switches_[enum_index(idx)]);
    }
    for (auto idx : magic_enum::enum_values<sensor_id_t>()) {
        _dump_if(this->sensors_[enum_index(idx)]);
    }
    for (auto idx : magic_enum::enum_values<binary_sensor_id_t>()) {
        _dump_if(this->binary_sensors_[enum_index(idx)]);
    }
    for (auto idx : magic_enum::enum_values<text_sensor_id_t>()) {
        if (idx != text_sensor_id_t::INTERFACE_FIRMWARE) {
            _dump_if(this->text_sensors_[enum_index(idx)]);
        }
    }
}

/**
 * @brief Updates climate entities with current pool state.
 *
 * @param[in] state Pointer to the current pool state.
 */
void
OpnPool::update_climates(const poolstate_t * const state)
{
    for (auto climate_id : magic_enum::enum_values<climate_id_t>()) {

        OpnPoolClimate * const climate = this->climates_[enum_index(climate_id)];
        if (!climate) continue;

        auto const water_temp = &state->temps[enum_index(poolstate_temp_typ_t::WATER)];
        auto const thermo_typ = climate->get_thermo_typ();
        auto const thermo = &state->thermos[enum_index(thermo_typ)];
        if (!water_temp->valid) continue;
        if (!thermo->set_point_in_f.valid) continue;
        if (!thermo->heat_src.valid) continue;
        if (!thermo->heating.valid) continue;

        auto const current_temp_f = water_temp->value;
        auto const current_temp_c = std::round(fahrenheit_to_celsius(current_temp_f) * 10.0f) / 10.0f;
        auto const target_temp_c = std::round(fahrenheit_to_celsius(thermo->set_point_in_f.value) * 10.0f) / 10.0f;

            // mode
        auto const switch_idx = _thermo_typ_to_pool_circuit_idx(thermo_typ);
        auto const active_circuit = &state->circuits[switch_idx].active;
        if (!active_circuit->valid) continue;

        climate::ClimateMode mode = active_circuit->value
                                  ? climate::CLIMATE_MODE_HEAT
                                  : climate::CLIMATE_MODE_OFF;

            // custom preset
        auto const custom_preset = enum_str(thermo->heat_src.value);

            // action
        climate::ClimateAction action = thermo->heating.value
                                      ? climate::CLIMATE_ACTION_HEATING
                                      : (mode == climate::CLIMATE_MODE_OFF
                                        ? climate::CLIMATE_ACTION_OFF
                                        : climate::CLIMATE_ACTION_IDLE);

        climate->publish_value_if_changed(current_temp_c, target_temp_c, mode, custom_preset, action);
    }
}

/**
 * @brief Updates switch entities with current pool state.
 *
 * @param[in] state Pointer to the current pool state.
 */
void
OpnPool::update_switches(const poolstate_t * const state)
{
    for (auto switch_id : magic_enum::enum_values<switch_id_t>()) {

        OpnPoolSwitch * const sw = this->switches_[enum_index(switch_id)];
        if (!sw) continue;

        auto const circuit = switch_id_to_network_circuit(switch_id);
        auto const active_circuit = &state->circuits[enum_index(circuit)].active;
        if (!active_circuit->valid) continue;

        sw->publish_value_if_changed(active_circuit->value);
    }
}

/**
 * @brief Updates analog sensor entities with current pool state.
 *
 * @param[in] state Pointer to the current pool state.
 */
void
OpnPool::update_analog_sensors(poolstate_t const * const state)
{
    OpnPoolSensor * const air_temp_sensor = this->sensors_[enum_index(sensor_id_t::AIR_TEMPERATURE)];
    auto const air_temp = state->temps[enum_index(poolstate_temp_typ_t::AIR)];
    if (air_temp_sensor != nullptr && air_temp.valid) {
        //auto air_temp_c = fahrenheit_to_celsius(air_temp.value);
        //air_temp_c = std::round(air_temp_c * 10.0f) / 10.0f;
        auto air_temp_f =std::round(air_temp.value * 10.0f) / 10.0f;
        air_temp_sensor->publish_value_if_changed(air_temp_f);    
    }

    OpnPoolSensor * const water_temperature_sensor = this->sensors_[enum_index(sensor_id_t::WATER_TEMPERATURE)];
    auto const water_temp = state->temps[enum_index(poolstate_temp_typ_t::WATER)];
    if (water_temperature_sensor != nullptr && water_temp.valid) {
        auto water_temp_f =std::round(water_temp.value * 10.0f) / 10.0f;
        water_temperature_sensor->publish_value_if_changed(water_temp_f);
    }

    OpnPoolSensor * const solar1_sensor = this->sensors_[enum_index(sensor_id_t::SOLAR1_TEMPERATURE)];
    auto const solar1_temp = state->temps[enum_index(poolstate_temp_typ_t::SOLAR_1)];
    if (solar1_sensor != nullptr && solar1_temp.valid) {
        auto solar1_temp_f = std::round(solar1_temp.value * 10.0f) / 10.0f;
        solar1_sensor->publish_value_if_changed(solar1_temp_f);
    }

    OpnPoolSensor * const solar2_sensor = this->sensors_[enum_index(sensor_id_t::SOLAR2_TEMPERATURE)];
    auto const solar2_temp = state->temps[enum_index(poolstate_temp_typ_t::SOLAR_2)];
    if (solar2_sensor != nullptr && solar2_temp.valid) {
        auto solar2_temp_f = std::round(solar2_temp.value * 10.0f) / 10.0f;
        solar2_sensor->publish_value_if_changed(solar2_temp_f);
    }   
    _publish_if(
        this->sensors_[enum_index(sensor_id_t::PRIMARY_PUMP_POWER)],        
        state->pumps[enum_index(datalink_pump_id_t::PRIMARY)].power
    );
    _publish_if(
        this->sensors_[enum_index(sensor_id_t::PRIMARY_PUMP_FLOW)],         
        state->pumps[enum_index(datalink_pump_id_t::PRIMARY)].flow
    );
    _publish_if(
        this->sensors_[enum_index(sensor_id_t::PRIMARY_PUMP_SPEED)],        
        state->pumps[enum_index(datalink_pump_id_t::PRIMARY)].speed
    );
    _publish_if(
        this->sensors_[enum_index(sensor_id_t::PRIMARY_PUMP_ERROR)],        
        state->pumps[enum_index(datalink_pump_id_t::PRIMARY)].error
    );
    _publish_if(
        this->sensors_[enum_index(sensor_id_t::CHLORINATOR_LEVEL)], 
        state->chlor.level
    );
    _publish_if(
        this->sensors_[enum_index(sensor_id_t::CHLORINATOR_SALT)],
        state->chlor.salt
    );

}

/**
 * @brief Updates binary sensor entities with current pool state.
 *
 * @param[in] state Pointer to the current pool state.
 */

void
OpnPool::update_binary_sensors(poolstate_t const * const state)
{
    _publish_if(
        this->binary_sensors_[enum_index(binary_sensor_id_t::PRIMARY_PUMP_POWER)],           
        state->pumps[enum_index(datalink_pump_id_t::PRIMARY)].running
    );
    _publish_modes_if(
        this->binary_sensors_,
        state->system.modes);
}

/**
 * @brief Updates number entities with current pool state.
 *
 * @param[in] state Pointer to the current pool state.
 */
void
OpnPool::update_numbers(poolstate_t const * const state)
{
    OpnPoolNumber * const speed_number = this->numbers_[enum_index(number_id_t::PRIMARY_PUMP_SPEED_SETPOINT)];
    auto const & speed = state->pumps[enum_index(datalink_pump_id_t::PRIMARY)].speed;
    if (speed_number != nullptr && speed.valid) {
        speed_number->publish_value_if_changed(static_cast<float>(speed.value));
    }

    OpnPoolNumber * const chlor_number = this->numbers_[enum_index(number_id_t::CHLORINATOR_SETPOINT)];
    auto const & chlor_level = state->chlor.level;
    if (chlor_number != nullptr && chlor_level.valid) {
        chlor_number->publish_value_if_changed(static_cast<float>(chlor_level.value));
    }
}

/**
 * @brief Updates text sensor entities with current pool state.
 *
 * @param[in] state Pointer to the current pool state.
 */
void
OpnPool::update_text_sensors(poolstate_t const * const state)
{
    _publish_schedule_if(
        this->text_sensors_[enum_index(text_sensor_id_t::POOL_SCHED)],
        &state->scheds[enum_index(network_pool_circuit_t::POOL)]
    );
    _publish_schedule_if(
        this->text_sensors_[enum_index(text_sensor_id_t::SPA_SCHED)],
        &state->scheds[enum_index(network_pool_circuit_t::SPA)]
    );
    _publish_enum_if(
        this->text_sensors_[enum_index(text_sensor_id_t::PRIMARY_PUMP_STATE)],
        state->pumps[enum_index(datalink_pump_id_t::PRIMARY)].state
    );
    _publish_str_if(
        this->text_sensors_[enum_index(text_sensor_id_t::PRIMARY_PUMP_MODE)], 
        state->pumps[enum_index(datalink_pump_id_t::PRIMARY)].mode
    );
    _publish_if(
        this->text_sensors_[enum_index(text_sensor_id_t::CHLORINATOR_NAME)], 
        state->chlor.name
    );
    _publish_enum_if(
        this->text_sensors_[enum_index(text_sensor_id_t::CHLORINATOR_STATUS)],
        state->chlor.status
    );
    _publish_date_and_time_if(
        this->text_sensors_[enum_index(text_sensor_id_t::SYSTEM_TIME)],
        &state->system.tod
    );
    _publish_version_if(
        this->text_sensors_[enum_index(text_sensor_id_t::CONTROLLER_TYPE)],
        &state->system
    );
}

// ============================================================================
// Setter methods (required by ESPHome's component model for code generation)
// ============================================================================

/**
 * @brief Sets the RS-485 GPIO pin assignments.
 *
 * @param[in] rx_pin  GPIO pin for RS-485 receive.
 * @param[in] tx_pin  GPIO pin for RS-485 transmit.
 * @param[in] rts_pin GPIO pin for RS-485 direction control.
 */
void
OpnPool::set_rs485_pins(uint8_t const rx_pin, uint8_t const tx_pin, uint8_t const rts_pin)
{
    rs485_pins_.rx_pin = rx_pin;
    rs485_pins_.tx_pin = tx_pin;
    rs485_pins_.rts_pin = rts_pin;
}

void
OpnPool::set_pool_climate(OpnPoolClimate * const climate)
{ 
    this->climates_[enum_index(poolstate_thermo_typ_t::POOL)] = climate; 
}

void
OpnPool::set_spa_climate(OpnPoolClimate * const climate)
{ 
    this->climates_[enum_index(poolstate_thermo_typ_t::SPA)] = climate; 
}

void
OpnPool::set_pool_switch(OpnPoolSwitch * const sw)
{ 
    this->switches_[enum_index(network_pool_circuit_t::POOL)] = sw; 
}

void
OpnPool::set_spa_switch(OpnPoolSwitch * const sw)
{ 
    this->switches_[enum_index(network_pool_circuit_t::SPA)] = sw; 
}

void
OpnPool::set_aux1_switch(OpnPoolSwitch * const sw)
{ 
    this->switches_[enum_index(network_pool_circuit_t::AUX1)] = sw; 
}

void
OpnPool::set_aux2_switch(OpnPoolSwitch * const sw) 
{ 
    this->switches_[enum_index(network_pool_circuit_t::AUX2)] = sw; 
}

void
OpnPool::set_aux3_switch(OpnPoolSwitch * const sw)
{ 
    this->switches_[enum_index(network_pool_circuit_t::AUX3)] = sw; 
}

void
OpnPool::set_feature1_switch(OpnPoolSwitch * const sw)
{ 
    this->switches_[enum_index(network_pool_circuit_t::FEATURE1)] = sw; 
}

void
OpnPool::set_feature2_switch(OpnPoolSwitch * const sw)
{ 
    this->switches_[enum_index(network_pool_circuit_t::FEATURE2)] = sw;
}

void
OpnPool::set_feature3_switch(OpnPoolSwitch * const sw)
{ 
    this->switches_[enum_index(network_pool_circuit_t::FEATURE3)] = sw; 
}

void
OpnPool::set_feature4_switch(OpnPoolSwitch * const sw)
{ 
    this->switches_[enum_index(network_pool_circuit_t::FEATURE4)] = sw; 
}

void
OpnPool::set_air_temperature_sensor(OpnPoolSensor * const s)
{ 
    this->sensors_[enum_index(sensor_id_t::AIR_TEMPERATURE)] = s; 
}

void
OpnPool::set_water_temperature_sensor(OpnPoolSensor * const s)
{
    this->sensors_[enum_index(sensor_id_t::WATER_TEMPERATURE)] = s;
}

void
OpnPool::set_solar1_temperature_sensor(OpnPoolSensor * const s)
{
    this->sensors_[enum_index(sensor_id_t::SOLAR1_TEMPERATURE)] = s;
}

void
OpnPool::set_solar2_temperature_sensor(OpnPoolSensor * const s)
{
    this->sensors_[enum_index(sensor_id_t::SOLAR2_TEMPERATURE)] = s;
}

void
OpnPool::set_primary_pump_power_sensor(OpnPoolSensor * const s)
{ 
    this->sensors_[enum_index(sensor_id_t::PRIMARY_PUMP_POWER)] = s; 
}

void
OpnPool::set_primary_pump_flow_sensor(OpnPoolSensor * const s)
{ 
    this->sensors_[enum_index(sensor_id_t::PRIMARY_PUMP_FLOW)] = s; 
}

void
OpnPool::set_primary_pump_speed_sensor(OpnPoolSensor * const s)
{ 
    this->sensors_[enum_index(sensor_id_t::PRIMARY_PUMP_SPEED)] = s; 
}

void
OpnPool::set_primary_pump_error_sensor(OpnPoolSensor * const s)
{ 
    this->sensors_[enum_index(sensor_id_t::PRIMARY_PUMP_ERROR)] = s; 
}

void
OpnPool::set_chlorinator_level_sensor(OpnPoolSensor * const s)
{ 
    this->sensors_[enum_index(sensor_id_t::CHLORINATOR_LEVEL)] = s; 
}

void
OpnPool::set_chlorinator_salt_sensor(OpnPoolSensor * const s)
{ 
    this->sensors_[enum_index(sensor_id_t::CHLORINATOR_SALT)] = s; 
}

void
OpnPool::set_primary_pump_speed_setpoint_number(OpnPoolNumber * const n)
{
    this->numbers_[enum_index(number_id_t::PRIMARY_PUMP_SPEED_SETPOINT)] = n;
}

void
OpnPool::set_chlorinator_setpoint_number(OpnPoolNumber * const n)
{
    this->numbers_[enum_index(number_id_t::CHLORINATOR_SETPOINT)] = n;
}

void
OpnPool::set_light_color_select(OpnPoolSelect * const s)
{
    this->selects_[enum_index(select_id_t::LIGHT_COLOR)] = s;
}

void
OpnPool::set_primary_pump_running_binary_sensor(OpnPoolBinarySensor * const bs)
{ 
    this->binary_sensors_[enum_index(binary_sensor_id_t::PRIMARY_PUMP_POWER)] = bs; 
}

void
OpnPool::set_mode_service_binary_sensor(OpnPoolBinarySensor * const bs)
{ 
    this->binary_sensors_[enum_index(binary_sensor_id_t::MODE_SERVICE)] = bs; 
}

void
OpnPool::set_mode_temperature_inc_binary_sensor(OpnPoolBinarySensor * const bs)
{ 
    this->binary_sensors_[enum_index(binary_sensor_id_t::MODE_TEMPERATURE_INC)] = bs; 
}

void
OpnPool::set_mode_freeze_protection_binary_sensor(OpnPoolBinarySensor * const bs)
{ 
    this->binary_sensors_[enum_index(binary_sensor_id_t::MODE_FREEZE_PROTECTION)] = bs; 
}

void
OpnPool::set_mode_timeout_binary_sensor(OpnPoolBinarySensor * const bs)
{ 
    this->binary_sensors_[enum_index(binary_sensor_id_t::MODE_TIMEOUT)] = bs; 
}

void
OpnPool::set_pool_sched_text_sensor(OpnPoolTextSensor * const ts) 
{ 
    this->text_sensors_[enum_index(text_sensor_id_t::POOL_SCHED)] = ts; 
}

void
OpnPool::set_spa_sched_text_sensor(OpnPoolTextSensor * const ts)
{ 
    this->text_sensors_[enum_index(text_sensor_id_t::SPA_SCHED)] = ts; 
}

void
OpnPool::set_primary_pump_mode_text_sensor(OpnPoolTextSensor * const ts)
{ 
    this->text_sensors_[enum_index(text_sensor_id_t::PRIMARY_PUMP_MODE)] = ts; 
}

void
OpnPool::set_primary_pump_state_text_sensor(OpnPoolTextSensor * const ts)
{ 
    this->text_sensors_[enum_index(text_sensor_id_t::PRIMARY_PUMP_STATE)] = ts; 
}

void
OpnPool::set_chlorinator_name_text_sensor(OpnPoolTextSensor * const ts)
{ 
    this->text_sensors_[enum_index(text_sensor_id_t::CHLORINATOR_NAME)] = ts; 
}

void
OpnPool::set_chlorinator_status_text_sensor(OpnPoolTextSensor * const ts)
{ 
    this->text_sensors_[enum_index(text_sensor_id_t::CHLORINATOR_STATUS)] = ts; 
}

void
OpnPool::set_system_time_text_sensor(OpnPoolTextSensor * const ts)
{ 
    this->text_sensors_[enum_index(text_sensor_id_t::SYSTEM_TIME)] = ts;  
}

void
OpnPool::set_controller_type_text_sensor(OpnPoolTextSensor * const ts)
{ 
    this->text_sensors_[enum_index(text_sensor_id_t::CONTROLLER_TYPE)] = ts; 
}

void
OpnPool::set_interface_firmware_text_sensor(OpnPoolTextSensor * const ts)
{
    this->text_sensors_[enum_index(text_sensor_id_t::INTERFACE_FIRMWARE)] = ts;
}

// ============================================================================
// Schedules (SCHEDS_SET opcode 0x91 verified on hardware via CTRL_SET_ACK)
// ============================================================================

void
OpnPool::send_schedule(uint8_t const sched_id, uint8_t const circuit_idx,
                       uint8_t const start_h, uint8_t const start_m,
                       uint8_t const stop_h, uint8_t const stop_m, uint8_t const day_of_week)
{
    if (poolState_ == nullptr) { return; }

    poolstate_t state;
    poolState_->get(&state);

    datalink_addr_t const controller_addr = state.system.addr.value;
    if (!controller_addr.is_controller()) {
        ESP_LOGW(TAG, "Controller address still unknown, cannot send schedule");
        return;
    }

    auto const circuit = static_cast<network_pool_circuit_t>(circuit_idx);

    network_msg_t msg = {};
    msg.src = datalink_addr_t::wireless_remote();  // 0x22 (ScreenLogic/wireless remote)
    msg.dst = controller_addr;
    msg.typ = network_msg_typ_t::CTRL_SCHEDS_SET;
    msg.u.a5.ctrl_scheds_set = {
        .sched_id       = sched_id,
        .circuit_plus_1 = static_cast<uint8_t>(circuit_idx + 1),  // controller uses circuit+1 (0x01 => SPA)
        .start          = { .hour = start_h, .minute = start_m },
        .stop           = { .hour = stop_h,  .minute = stop_m  },
        .day_of_week    = day_of_week,
    };

    ESP_LOGI(TAG, "SCHEDS_SET id=%u circuit=%s(+1=%u) %02u:%02u-%02u:%02u days=0x%02X",
             sched_id, enum_str(circuit), circuit_idx + 1, start_h, start_m, stop_h, stop_m, day_of_week);

    if (ipc_send_network_msg_to_pool_task(&msg, ipc_) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to queue SCHEDS_SET message");
    }
}

#ifdef USE_API_CUSTOM_SERVICES

// ---- minimal YAML-subset helpers for the export/import services ----

/**
 * @brief Renders a day-of-week bitmask as a 7-char positional string (Sun→Sat).
 *
 * @details Bit i maps to string position i: bit0 Sun, bit1 Mon, bit2 Tue, bit3 Wed,
 *          bit4 Thu, bit5 Fri, bit6 Sat (confirmed against the controller; Mon-Fri = 0x3E).
 *
 * @param[in]  mask Sunday-first day bitmask.
 * @param[out] out  Buffer of at least 8 bytes; receives e.g. "-MTWTF-" + NUL.
 */
static void
_days_mask_to_str(uint8_t const mask, char * const out)
{
    static char const letters[7] = {'S', 'M', 'T', 'W', 'T', 'F', 'S'};  // Sun..Sat
    for (int i = 0; i < 7; i++) {
        out[i] = (mask & (1 << i)) ? letters[i] : '-';
    }
    out[7] = '\0';
}

/// @brief Strips surrounding whitespace and double-quotes from a token.
static std::string
_trim(std::string const & s)
{
    size_t const a = s.find_first_not_of(" \t\r\n\"");
    if (a == std::string::npos) { return ""; }
    size_t const b = s.find_last_not_of(" \t\r\n\"");
    return s.substr(a, b - a + 1);
}

/// @brief Parses "HH:MM" into hour/minute; returns false if out of range or malformed.
static bool
_parse_hhmm(std::string const & v, uint8_t & h, uint8_t & m)
{
    size_t const colon = v.find(':');
    if (colon == std::string::npos) { return false; }
    int const hh = atoi(v.substr(0, colon).c_str());
    int const mm = atoi(v.substr(colon + 1).c_str());
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59) { return false; }
    h = static_cast<uint8_t>(hh);
    m = static_cast<uint8_t>(mm);
    return true;
}

/// @brief Converts a 7-char positional day string (Mon→Sun) to a bitmask. Non-'-' = on.
static uint8_t
_days_str_to_mask(std::string const & v)
{
    uint8_t mask = 0;
    for (size_t i = 0; i < v.size() && i < 7; i++) {
        char const c = v[i];
        if (c != '-' && c != ' ' && c != '_') { mask |= static_cast<uint8_t>(1 << i); }
    }
    return mask;
}

/// @brief Resolves a circuit name (e.g. "POOL") to its network_pool_circuit_t index.
static bool
_circuit_idx_from_name(std::string name, uint8_t & out_idx)
{
    for (auto & c : name) { c = static_cast<char>(toupper(static_cast<unsigned char>(c))); }
    for (auto const circuit : magic_enum::enum_values<network_pool_circuit_t>()) {
        if (name == enum_str(circuit)) {
            out_idx = enum_index(circuit);
            return true;
        }
    }
    return false;
}

void
OpnPool::on_export_schedules()
{
    if (poolState_ == nullptr) { return; }

    poolstate_t state;
    poolState_->get(&state);

    std::string doc = "schedules:\n";
    char line[128];
    int count = 0;

    for (auto const & s : state.scheds_detail) {
        if (!s.valid) { continue; }
            // skip unused slots (circuit_plus_1 == 0 means no circuit assigned)
        if (s.circuit_plus_1 == 0 ||
            s.circuit_plus_1 > enum_count<network_pool_circuit_t>()) { continue; }

        auto const circuit = static_cast<network_pool_circuit_t>(s.circuit_plus_1 - 1);
        char days[8];
        _days_mask_to_str(s.day_of_week, days);
        snprintf(line, sizeof(line),
                 "  - {id: %u, circuit: %s, start: \"%02u:%02u\", stop: \"%02u:%02u\", days: \"%s\"}\n",
                 s.sched_id, enum_str(circuit),
                 s.start.hour, s.start.minute, s.stop.hour, s.stop.minute, days);
        doc += line;
        count++;
    }
    if (count == 0) { doc += "  []  # no schedules known yet\n"; }

    ESP_LOGI(TAG, "Exported %d schedule(s):\n%s", count, doc.c_str());
#ifdef USE_API_HOMEASSISTANT_SERVICES
        // also surface the YAML to HA as an event (requires 'homeassistant_services: true')
    fire_homeassistant_event("esphome.opnpool_schedules_export", {{"yaml", doc}});
#endif
}

void
OpnPool::on_import_schedule(std::string yaml)
{
    int applied = 0;
    int skipped = 0;
    size_t pos = 0;

    while (pos < yaml.size()) {
        size_t const eol = yaml.find('\n', pos);
        std::string const ln = yaml.substr(pos, eol == std::string::npos ? std::string::npos : eol - pos);
        pos = (eol == std::string::npos) ? yaml.size() : eol + 1;

            // an entry is a flow-style map: - {id: .., circuit: .., start: .., stop: .., days: ..}
        size_t const lb = ln.find('{');
        if (lb == std::string::npos) { continue; }
        size_t const rb = ln.find('}', lb);
        if (rb == std::string::npos) { continue; }
        std::string const body = ln.substr(lb + 1, rb - lb - 1);

        int     id          = -1;
        int     circuit_idx = -1;
        uint8_t sh = 0, sm = 0, eh = 0, em = 0;
        uint8_t days = 0;
        bool    have_start = false, have_stop = false, have_days = false;

        size_t tpos = 0;
        while (tpos < body.size()) {
            size_t const comma = body.find(',', tpos);
            std::string const tok = body.substr(tpos, comma == std::string::npos ? std::string::npos : comma - tpos);
            tpos = (comma == std::string::npos) ? body.size() : comma + 1;

            size_t const colon = tok.find(':');
            if (colon == std::string::npos) { continue; }
            std::string key = _trim(tok.substr(0, colon));
            std::string const val = _trim(tok.substr(colon + 1));
            for (auto & c : key) { c = static_cast<char>(tolower(static_cast<unsigned char>(c))); }

            if (key == "id") {
                id = atoi(val.c_str());
            } else if (key == "circuit") {
                uint8_t ci;
                if (_circuit_idx_from_name(val, ci)) { circuit_idx = ci; }
            } else if (key == "start") {
                have_start = _parse_hhmm(val, sh, sm);
            } else if (key == "stop") {
                have_stop = _parse_hhmm(val, eh, em);
            } else if (key == "days") {
                days = _days_str_to_mask(val);
                have_days = true;
            }
        }

        if (id < 1 || id > NETWORK_CTRL_SCHEDS_COUNT || circuit_idx < 0 || !have_start || !have_stop) {
            ESP_LOGW(TAG, "import_schedule: skipping invalid entry: %s", ln.c_str());
            skipped++;
            continue;
        }
        if (!have_days) { days = 0x7F; }  // default: every day

        this->send_schedule(static_cast<uint8_t>(id), static_cast<uint8_t>(circuit_idx),
                            sh, sm, eh, em, days);
        applied++;
    }

    ESP_LOGI(TAG, "import_schedule: applied=%d skipped=%d", applied, skipped);
}

#endif  // USE_API_CUSTOM_SERVICES

#ifdef USE_MATTER
/**
 * @brief Configure Matter over Thread settings.
 *
 * @param[in] discriminator Device discriminator for pairing (0-4095).
 * @param[in] passcode      Setup passcode for commissioning (1-99999998).
 */
void
OpnPool::set_matter_config(uint16_t const discriminator, uint32_t const passcode)
{
    matter_config_.discriminator = discriminator;
    matter_config_.passcode = passcode;
    ESP_LOGI(TAG, "Matter config set: discriminator=%u, passcode=%lu",
             discriminator, static_cast<unsigned long>(passcode));
}

/**
 * @brief Check if Matter device is commissioned to a fabric.
 *
 * @return True if commissioned, false otherwise.
 */
bool
OpnPool::is_matter_commissioned() const
{
    return matter_bridge_ != nullptr && matter_bridge_->is_commissioned();
}

/**
 * @brief Get Matter QR code payload for commissioning.
 *
 * @param[out] buf      Buffer to write QR code string.
 * @param[in]  buf_size Size of buffer.
 * @return              Length of QR code string, or 0 on error.
 */
size_t
OpnPool::get_matter_qr_code(char * buf, size_t buf_size) const
{
    if (matter_bridge_ == nullptr) {
        return 0;
    }
    return matter_bridge_->get_qr_code(buf, buf_size);
}
#endif  // USE_MATTER

}  // namespace opnpool
}  // namespace esphome