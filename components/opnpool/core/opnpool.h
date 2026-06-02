/**
 * @file opnpool.h
 * @brief Main OPNpool component interface for ESPHome.
 *
 * @details
 * Declares the OpnPool class, the main ESPHome component that provides
 * bidirectional integration between the pool controller and Home Assistant.
 * It manages climate, switch, sensor, binary sensor, and text sensor entities,
 * spawns the pool_task for RS-485 communication, and handles state updates.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#ifndef ESPHOME_OPNPOOL_OPNPOOL_H_
#define ESPHOME_OPNPOOL_OPNPOOL_H_
#ifndef __cplusplus
# error "Requires C++ compilation"
#endif

#include <esp_system.h>
#include <esp_types.h>
#include <string>
#include <esphome/core/component.h>
#include <esphome/core/defines.h>

#include "utils/enum_helpers.h"
#include "opnpool_ids.h"

#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif

#ifdef USE_MATTER
#include "matter/matter_bridge.h"
#endif

namespace esphome {
namespace opnpool {

// Forward declarations (to avoid circular dependencies)
struct ipc_t;
struct poolstate_t;
struct pending_switch_t;
struct pending_climate_t;
class PoolState;
class OpnPoolClimate;
class OpnPoolSwitch;
class OpnPoolSensor;
class OpnPoolBinarySensor;
class OpnPoolTextSensor;
class OpnPoolNumber;
class OpnPoolSelect;

/// @brief RS-485 GPIO pin configuration.
struct rs485_pins_t {
    uint8_t rx_pin{21};   ///< Receive pin GPIO number.
    uint8_t tx_pin{22};   ///< Transmit pin GPIO number.
    uint8_t rts_pin{23};  ///< Direction control (RTS) pin GPIO number.
};

/**
 * @brief Main OPNpool component for ESPHome.
 *
 * @details
 * Provides bidirectional integration between pool controller hardware and the
 * ESPHome ecosystem. Publishes pool state changes to Home Assistant entities
 * and enacts control requests from Home Assistant on the physical equipment.
 */
class OpnPool : public Component
#ifdef USE_API
              , public api::CustomAPIDevice
#endif
{

  public:
    void setup() override;      ///< Initializes IPC, spawns pool_task, publishes firmware version.
    void loop() override;       ///< Processes messages from pool_task, updates entities.
    void dump_config() override;///< Logs component configuration.
    ~OpnPool();                 ///< Cleans up resources.

    // ========== RS-485 Configuration ==========
    void set_rs485_pins(uint8_t rx_pin, uint8_t tx_pin, uint8_t rts_pin);

    // ========== Climate Setters ==========
    void set_pool_climate(OpnPoolClimate * const climate);
    void set_spa_climate(OpnPoolClimate * const climate);

    // ========== Switch Setters ==========
    void set_pool_switch(OpnPoolSwitch * const sw);
    void set_spa_switch(OpnPoolSwitch * const sw);
    void set_aux1_switch(OpnPoolSwitch * const sw);
    void set_aux2_switch(OpnPoolSwitch * const sw);
    void set_aux3_switch(OpnPoolSwitch * const sw);
    void set_feature1_switch(OpnPoolSwitch * const sw);
    void set_feature2_switch(OpnPoolSwitch * const sw);
    void set_feature3_switch(OpnPoolSwitch * const sw);
    void set_feature4_switch(OpnPoolSwitch * const sw);

    // ========== Sensor Setters ==========
    void set_air_temperature_sensor(OpnPoolSensor * const s);
    void set_water_temperature_sensor(OpnPoolSensor * const s);
    void set_solar1_temperature_sensor(OpnPoolSensor * const s);
    void set_solar2_temperature_sensor(OpnPoolSensor * const s);
    void set_primary_pump_power_sensor(OpnPoolSensor * const s);
    void set_primary_pump_flow_sensor(OpnPoolSensor * const s);
    void set_primary_pump_speed_sensor(OpnPoolSensor * const s);
    void set_primary_pump_error_sensor(OpnPoolSensor * const s);
    void set_chlorinator_level_sensor(OpnPoolSensor * const s);
    void set_chlorinator_salt_sensor(OpnPoolSensor * const s);

    // ========== Binary Sensor Setters ==========
    void set_primary_pump_running_binary_sensor(OpnPoolBinarySensor * const bs);
    void set_mode_service_binary_sensor(OpnPoolBinarySensor * const bs);
    void set_mode_temperature_inc_binary_sensor(OpnPoolBinarySensor * const bs);
    void set_mode_freeze_protection_binary_sensor(OpnPoolBinarySensor * const bs);
    void set_mode_timeout_binary_sensor(OpnPoolBinarySensor * const bs);

    // ========== Number Setters ==========
    void set_primary_pump_speed_setpoint_number(OpnPoolNumber * const n);
    void set_chlorinator_setpoint_number(OpnPoolNumber * const n);

    // ========== Select Setters ==========
    void set_light_color_select(OpnPoolSelect * const s);

    // ========== Text Sensor Setters ==========
    void set_pool_sched_text_sensor(OpnPoolTextSensor * const ts);
    void set_spa_sched_text_sensor(OpnPoolTextSensor * const ts);
    void set_primary_pump_mode_text_sensor(OpnPoolTextSensor * const ts);
    void set_primary_pump_state_text_sensor(OpnPoolTextSensor * const ts);
    void set_chlorinator_name_text_sensor(OpnPoolTextSensor * const ts);
    void set_chlorinator_status_text_sensor(OpnPoolTextSensor * const ts);
    void set_system_time_text_sensor(OpnPoolTextSensor * const ts);
    void set_controller_type_text_sensor(OpnPoolTextSensor * const ts);
    void set_interface_firmware_text_sensor(OpnPoolTextSensor * const ts);

#ifdef USE_MATTER
    // ========== Matter Configuration ==========
    void set_matter_config(uint16_t discriminator, uint32_t passcode);

    /// @brief Check if Matter is enabled and initialized.
    [[nodiscard]] bool is_matter_enabled() const { return matter_bridge_ != nullptr; }

    /// @brief Check if Matter device is commissioned to a fabric.
    [[nodiscard]] bool is_matter_commissioned() const;

    /// @brief Get Matter QR code for commissioning.
    size_t get_matter_qr_code(char * buf, size_t buf_size) const;
#endif

    // ========== Entity Update Methods ==========
    void update_climates(poolstate_t const * const state);
    void update_switches(poolstate_t const * const state);
    void update_text_sensors(poolstate_t const * const state);
    void update_analog_sensors(poolstate_t const * const state);
    void update_binary_sensors(poolstate_t const * const state);
    void update_numbers(poolstate_t const * const state);
    void update_all(poolstate_t const * const state);

    // ========== Schedules (SCHEDS_SET 0x91 verified on hardware) ==========

    /**
     * @brief Sends a CTRL_SCHEDS_SET to write one detailed (EasyTouch) schedule slot.
     *
     * @param[in] sched_id    Schedule slot id (1..NETWORK_CTRL_SCHEDS_COUNT).
     * @param[in] circuit_idx Circuit index (network_pool_circuit_t value).
     * @param[in] start_h     Start hour (0–23).
     * @param[in] start_m     Start minute (0–59).
     * @param[in] stop_h      Stop hour (0–23).
     * @param[in] stop_m      Stop minute (0–59).
     * @param[in] day_of_week Day bitmask (Mon 0x01 .. Sun 0x40).
     */
    void send_schedule(uint8_t sched_id, uint8_t circuit_idx,
                       uint8_t start_h, uint8_t start_m,
                       uint8_t stop_h, uint8_t stop_m, uint8_t day_of_week);

#ifdef USE_API_CUSTOM_SERVICES
    // ========== Home Assistant services (require 'custom_services: true' in api:) ==========
    /// @brief HA service: log the controller's current schedules as YAML and fire an event.
    void on_export_schedules();
    /// @brief HA service: parse a schedules YAML document and write each entry to the controller.
    void on_import_schedule(std::string yaml);
#endif

    // ========== Accessors ==========
    ipc_t *         get_ipc()              { return ipc_; }                 ///< Returns IPC structure pointer.
    PoolState *     get_opnpool_state()    { return poolState_; }           ///< Returns pool state pointer.
    OpnPoolSwitch * get_switch(uint8_t id) { return this->switches_[id]; }  ///< Returns switch by ID.
    
  protected:
    rs485_pins_t rs485_pins_;                ///< RS-485 GPIO pin configuration.
    ipc_t * ipc_{nullptr};                   ///< IPC structure for task communication.
    PoolState * poolState_{nullptr};         ///< Pool state manager instance.
    TaskHandle_t pool_task_handle_{nullptr}; ///< FreeRTOS task handle for pool_task.

    // ========== Entity Arrays ==========
    OpnPoolClimate * climates_[enum_count<climate_id_t>()]{nullptr};              ///< Climate entity pointers.
    OpnPoolSwitch * switches_[enum_count<switch_id_t>()]{nullptr};                ///< Switch entity pointers.
    OpnPoolSensor * sensors_[enum_count<sensor_id_t>()]{nullptr};                 ///< Sensor entity pointers.
    OpnPoolBinarySensor * binary_sensors_[enum_count<binary_sensor_id_t>()]{nullptr}; ///< Binary sensor pointers.
    OpnPoolTextSensor * text_sensors_[enum_count<text_sensor_id_t>()]{nullptr};   ///< Text sensor pointers.
    OpnPoolNumber * numbers_[enum_count<number_id_t>()]{nullptr};                 ///< Number entity pointers.
    OpnPoolSelect * selects_[enum_count<select_id_t>()]{nullptr};                 ///< Select entity pointers.

#ifdef USE_MATTER
    // ========== Matter Integration ==========
    matter::MatterBridge * matter_bridge_{nullptr};  ///< Matter bridge instance.
    matter::matter_config_t matter_config_{};        ///< Matter configuration.
#endif
};

} // namespace opnpool
} // namespace esphome

#endif // ESPHOME_OPNPOOL_OPNPOOL_H_