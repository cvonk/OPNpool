/**
 * @file matter_bridge.cpp
 * @brief Matter over Thread bridge implementation.
 *
 * @details
 * Implements the Matter bridge layer for OPNpool, creating Matter endpoints
 * for pool thermostats, circuit switches, and temperature sensors. Handles
 * bidirectional attribute synchronization and command queuing.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifdef USE_MATTER

#include "matter_bridge.h"

#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_attribute_utils.h>
#include <esp_matter_core.h>
#include <esp_matter_cluster.h>
#include <esp_matter_endpoint.h>
#include <esp_matter_identify.h>

#include <app/clusters/identify-server/identify-server.h>
#include <app/server/Server.h>
#include <credentials/FabricTable.h>

#include "../pool_task/network.h"
#include "../utils/to_str.h"

namespace esphome {
namespace opnpool {
namespace matter {

using namespace esp_matter;
using namespace chip::app::Clusters;

constexpr char TAG[] = "matter_bridge";

constexpr size_t PENDING_CMD_QUEUE_LEN = 10;

/// @name Matter Bridge Implementation
/// @{

MatterBridge::~MatterBridge()
{
    if (pending_commands_q_ != nullptr) {
        vQueueDelete(pending_commands_q_);
        pending_commands_q_ = nullptr;
    }
    // Note: esp_matter doesn't have a clean shutdown API in current versions
}

esp_err_t
MatterBridge::init(matter_config_t const * config)
{
    if (config == nullptr) {
        ESP_LOGE(TAG, "Null config");
        return ESP_ERR_INVALID_ARG;
    }

    config_ = *config;

    ESP_LOGI(TAG, "Initializing Matter bridge (discriminator=%u)", config_.discriminator);

    // Create pending commands queue
    pending_commands_q_ = xQueueCreate(PENDING_CMD_QUEUE_LEN, sizeof(network_msg_t));
    if (pending_commands_q_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create pending commands queue");
        return ESP_ERR_NO_MEM;
    }

    // Initialize Matter node
    node::config_t node_config;
    node_ = node::create(&node_config, attribute_update_callback, identification_callback);
    if (node_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        vQueueDelete(pending_commands_q_);
        pending_commands_q_ = nullptr;
        return ESP_FAIL;
    }

    // Set discriminator and passcode
    // Note: In production, these should be stored in NVS and provisioned per-device
    esp_matter::set_setup_discriminator(config_.discriminator);
    esp_matter::set_setup_passcode(config_.passcode);

    // Create root node endpoint (required by Matter)
    endpoint::root_node::config_t root_config;
    endpoint::root_node::create(node_, &root_config, ENDPOINT_FLAG_NONE, this);

    // Create pool-specific endpoints
    esp_err_t err = create_thermostat_endpoints();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create thermostat endpoints: %s", esp_err_to_name(err));
        return err;
    }

    err = create_circuit_endpoints();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create circuit endpoints: %s", esp_err_to_name(err));
        return err;
    }

    err = create_temperature_sensor_endpoints();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create temperature sensor endpoints: %s", esp_err_to_name(err));
        return err;
    }

    // Start Matter
    err = esp_matter::start(attribute_update_callback);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Matter: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Matter bridge initialized successfully");
    return ESP_OK;
}

esp_err_t
MatterBridge::create_thermostat_endpoints()
{
    // Pool thermostat
    {
        endpoint::thermostat::config_t thermo_config;
        thermo_config.thermostat.local_temperature = 2500;           // 25.00°C (in 0.01°C units)
        thermo_config.thermostat.occupied_heating_setpoint = 2700;   // 27.00°C
        thermo_config.thermostat.system_mode = 0;                    // Off
        thermo_config.thermostat.control_sequence_of_operation = 2;  // Heating only

        endpoint_t * ep = endpoint::thermostat::create(node_, &thermo_config, ENDPOINT_FLAG_NONE, this);
        if (ep == nullptr) {
            ESP_LOGE(TAG, "Failed to create pool thermostat endpoint");
            return ESP_FAIL;
        }

        thermostat_endpoints_[MATTER_POOL_THERMO_IDX] = endpoint::get_id(ep);
        ESP_LOGI(TAG, "Created pool thermostat endpoint: %u", thermostat_endpoints_[MATTER_POOL_THERMO_IDX]);

        // Set friendly name via Basic Information cluster
        cluster_t * basic_cluster = cluster::get(ep, BasicInformation::Id);
        if (basic_cluster) {
            char node_label[] = "Pool Heater";
            attribute::set_val(
                attribute::get(basic_cluster, BasicInformation::Attributes::NodeLabel::Id),
                esp_matter_char_str(node_label, sizeof(node_label) - 1)
            );
        }
    }

    // Spa thermostat
    {
        endpoint::thermostat::config_t thermo_config;
        thermo_config.thermostat.local_temperature = 3800;           // 38.00°C
        thermo_config.thermostat.occupied_heating_setpoint = 4000;   // 40.00°C
        thermo_config.thermostat.system_mode = 0;
        thermo_config.thermostat.control_sequence_of_operation = 2;

        endpoint_t * ep = endpoint::thermostat::create(node_, &thermo_config, ENDPOINT_FLAG_NONE, this);
        if (ep == nullptr) {
            ESP_LOGE(TAG, "Failed to create spa thermostat endpoint");
            return ESP_FAIL;
        }

        thermostat_endpoints_[MATTER_SPA_THERMO_IDX] = endpoint::get_id(ep);
        ESP_LOGI(TAG, "Created spa thermostat endpoint: %u", thermostat_endpoints_[MATTER_SPA_THERMO_IDX]);

        cluster_t * basic_cluster = cluster::get(ep, BasicInformation::Id);
        if (basic_cluster) {
            char node_label[] = "Spa Heater";
            attribute::set_val(
                attribute::get(basic_cluster, BasicInformation::Attributes::NodeLabel::Id),
                esp_matter_char_str(node_label, sizeof(node_label) - 1)
            );
        }
    }

    return ESP_OK;
}

esp_err_t
MatterBridge::create_circuit_endpoints()
{
    // Circuit names matching network_pool_circuit_t order
    static const char * const circuit_names[MATTER_NUM_CIRCUITS] = {
        "Spa",
        "Aux 1",
        "Aux 2",
        "Aux 3",
        "Feature 1",
        "Pool",
        "Feature 2",
        "Feature 3",
        "Feature 4"
    };

    for (size_t i = 0; i < MATTER_NUM_CIRCUITS; i++) {
        endpoint::on_off_plugin_unit::config_t switch_config;
        switch_config.on_off.on_off = false;

        endpoint_t * ep = endpoint::on_off_plugin_unit::create(node_, &switch_config, ENDPOINT_FLAG_NONE, this);
        if (ep == nullptr) {
            ESP_LOGE(TAG, "Failed to create circuit endpoint %zu", i);
            return ESP_FAIL;
        }

        circuit_endpoints_[i] = endpoint::get_id(ep);
        ESP_LOGI(TAG, "Created circuit '%s' endpoint: %u", circuit_names[i], circuit_endpoints_[i]);

        // Set friendly name
        cluster_t * basic_cluster = cluster::get(ep, BasicInformation::Id);
        if (basic_cluster) {
            attribute::set_val(
                attribute::get(basic_cluster, BasicInformation::Attributes::NodeLabel::Id),
                esp_matter_char_str(const_cast<char *>(circuit_names[i]), strlen(circuit_names[i]))
            );
        }
    }

    return ESP_OK;
}

esp_err_t
MatterBridge::create_temperature_sensor_endpoints()
{
    static const char * const sensor_names[MATTER_NUM_TEMP_SENSORS] = {
        "Water Temperature",
        "Air Temperature"
    };

    for (size_t i = 0; i < MATTER_NUM_TEMP_SENSORS; i++) {
        endpoint::temperature_sensor::config_t sensor_config;
        sensor_config.temperature_measurement.measured_value = 2500;  // 25.00°C
        sensor_config.temperature_measurement.min_measured_value = -1000;  // -10.00°C
        sensor_config.temperature_measurement.max_measured_value = 6000;   // 60.00°C

        endpoint_t * ep = endpoint::temperature_sensor::create(node_, &sensor_config, ENDPOINT_FLAG_NONE, this);
        if (ep == nullptr) {
            ESP_LOGE(TAG, "Failed to create temperature sensor endpoint %zu", i);
            return ESP_FAIL;
        }

        temp_sensor_endpoints_[i] = endpoint::get_id(ep);
        ESP_LOGI(TAG, "Created temperature sensor '%s' endpoint: %u", sensor_names[i], temp_sensor_endpoints_[i]);

        cluster_t * basic_cluster = cluster::get(ep, BasicInformation::Id);
        if (basic_cluster) {
            attribute::set_val(
                attribute::get(basic_cluster, BasicInformation::Attributes::NodeLabel::Id),
                esp_matter_char_str(const_cast<char *>(sensor_names[i]), strlen(sensor_names[i]))
            );
        }
    }

    return ESP_OK;
}

void
MatterBridge::update_from_poolstate(poolstate_t const * state)
{
    if (state == nullptr || node_ == nullptr) {
        return;
    }

    // Update thermostat local temperatures from water temperature
    if (state->temps[enum_index(poolstate_temp_typ_t::WATER)].valid) {
        int16_t temp_centi_c = static_cast<int16_t>(
            matter_fahrenheit_to_celsius(
                static_cast<float>(state->temps[enum_index(poolstate_temp_typ_t::WATER)].value)
            ) * 100.0f
        );

        esp_matter_attr_val_t val = esp_matter_nullable_int16(temp_centi_c);

        // Update both thermostat endpoints
        for (size_t i = 0; i < MATTER_NUM_THERMOSTATS; i++) {
            attribute::update(
                thermostat_endpoints_[i],
                Thermostat::Id,
                Thermostat::Attributes::LocalTemperature::Id,
                &val
            );
        }

        // Update water temperature sensor
        attribute::update(
            temp_sensor_endpoints_[0],  // Water temp sensor
            TemperatureMeasurement::Id,
            TemperatureMeasurement::Attributes::MeasuredValue::Id,
            &val
        );
    }

    // Update air temperature sensor
    if (state->temps[enum_index(poolstate_temp_typ_t::AIR)].valid) {
        int16_t temp_centi_c = static_cast<int16_t>(
            matter_fahrenheit_to_celsius(
                static_cast<float>(state->temps[enum_index(poolstate_temp_typ_t::AIR)].value)
            ) * 100.0f
        );

        esp_matter_attr_val_t val = esp_matter_nullable_int16(temp_centi_c);
        attribute::update(
            temp_sensor_endpoints_[1],  // Air temp sensor
            TemperatureMeasurement::Id,
            TemperatureMeasurement::Attributes::MeasuredValue::Id,
            &val
        );
    }

    // Update thermostat setpoints
    for (size_t i = 0; i < MATTER_NUM_THERMOSTATS; i++) {
        auto const & thermo = state->thermos[i];
        if (thermo.set_point_in_f.valid) {
            int16_t setpoint_centi_c = static_cast<int16_t>(
                matter_fahrenheit_to_celsius(
                    static_cast<float>(thermo.set_point_in_f.value)
                ) * 100.0f
            );

            esp_matter_attr_val_t val = esp_matter_int16(setpoint_centi_c);
            attribute::update(
                thermostat_endpoints_[i],
                Thermostat::Id,
                Thermostat::Attributes::OccupiedHeatingSetpoint::Id,
                &val
            );
        }
    }

    // Update thermostat system mode based on circuit active state
    // Pool thermostat mode depends on POOL circuit
    uint8_t pool_circuit_idx = 5;  // network_pool_circuit_t::POOL = 5
    if (state->circuits[pool_circuit_idx].active.valid) {
        uint8_t mode = state->circuits[pool_circuit_idx].active.value ? 4 : 0;  // 4=Heat, 0=Off
        esp_matter_attr_val_t val = esp_matter_enum8(mode);
        attribute::update(
            thermostat_endpoints_[MATTER_POOL_THERMO_IDX],
            Thermostat::Id,
            Thermostat::Attributes::SystemMode::Id,
            &val
        );
    }

    // Spa thermostat mode depends on SPA circuit
    uint8_t spa_circuit_idx = 0;  // network_pool_circuit_t::SPA = 0
    if (state->circuits[spa_circuit_idx].active.valid) {
        uint8_t mode = state->circuits[spa_circuit_idx].active.value ? 4 : 0;
        esp_matter_attr_val_t val = esp_matter_enum8(mode);
        attribute::update(
            thermostat_endpoints_[MATTER_SPA_THERMO_IDX],
            Thermostat::Id,
            Thermostat::Attributes::SystemMode::Id,
            &val
        );
    }

    // Update circuit OnOff states
    for (size_t i = 0; i < MATTER_NUM_CIRCUITS; i++) {
        if (state->circuits[i].active.valid) {
            esp_matter_attr_val_t val = esp_matter_bool(state->circuits[i].active.value);
            attribute::update(
                circuit_endpoints_[i],
                OnOff::Id,
                OnOff::Attributes::OnOff::Id,
                &val
            );
        }
    }
}

bool
MatterBridge::get_pending_command(network_msg_t * msg)
{
    if (pending_commands_q_ == nullptr || msg == nullptr) {
        return false;
    }
    return xQueueReceive(pending_commands_q_, msg, 0) == pdPASS;
}

bool
MatterBridge::is_commissioned() const
{
    // Check if device has any fabrics
    auto & fabric_table = chip::Server::GetInstance().GetFabricTable();
    return fabric_table.FabricCount() > 0;
}

size_t
MatterBridge::get_qr_code(char * buf, size_t buf_size) const
{
    if (buf == nullptr || buf_size == 0) {
        return 0;
    }

    // Generate QR code payload
    // Format: MT:<vendor>:<product>:<discriminator>:<passcode>
    int len = snprintf(buf, buf_size,
        "MT:Y.K9042PS006%04X%08lu",
        config_.discriminator,
        static_cast<unsigned long>(config_.passcode));

    return (len > 0 && static_cast<size_t>(len) < buf_size) ? static_cast<size_t>(len) : 0;
}

int
MatterBridge::find_circuit_index(uint16_t endpoint_id) const
{
    for (size_t i = 0; i < MATTER_NUM_CIRCUITS; i++) {
        if (circuit_endpoints_[i] == endpoint_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int
MatterBridge::find_thermostat_index(uint16_t endpoint_id) const
{
    for (size_t i = 0; i < MATTER_NUM_THERMOSTATS; i++) {
        if (thermostat_endpoints_[i] == endpoint_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

esp_err_t
MatterBridge::handle_onoff_write(uint16_t endpoint_id, bool value)
{
    int circuit_idx = find_circuit_index(endpoint_id);
    if (circuit_idx < 0) {
        ESP_LOGW(TAG, "Unknown circuit endpoint: %u", endpoint_id);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Matter OnOff write: circuit %d = %s", circuit_idx, value ? "ON" : "OFF");

    // Create network message for pool controller
    network_msg_t msg = {};
    msg.typ = network_msg_typ_t::CTRL_CIRCUIT_SET;
    msg.u.a5.ctrl_circuit_set.circuit_plus_1 = static_cast<uint8_t>(circuit_idx + 1);
    msg.u.a5.ctrl_circuit_set.set_value(value);

    if (xQueueSend(pending_commands_q_, &msg, 0) != pdPASS) {
        ESP_LOGW(TAG, "Failed to queue circuit command");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t
MatterBridge::handle_thermostat_setpoint_write(uint16_t endpoint_id, int16_t temp_centi_c)
{
    int thermo_idx = find_thermostat_index(endpoint_id);
    if (thermo_idx < 0) {
        ESP_LOGW(TAG, "Unknown thermostat endpoint: %u", endpoint_id);
        return ESP_ERR_NOT_FOUND;
    }

    // Convert from 0.01°C to °F
    float temp_c = static_cast<float>(temp_centi_c) / 100.0f;
    uint8_t temp_f = static_cast<uint8_t>(matter_celsius_to_fahrenheit(temp_c));

    ESP_LOGI(TAG, "Matter thermostat setpoint write: thermo %d = %u°F (%.1f°C)",
             thermo_idx, temp_f, temp_c);

    // Create network message for pool controller
    network_msg_t msg = {};
    msg.typ = network_msg_typ_t::CTRL_HEAT_SET;

    // Set the appropriate setpoint based on thermostat index
    if (thermo_idx == MATTER_POOL_THERMO_IDX) {
        msg.u.a5.ctrl_heat_set.pool_set_point = temp_f;
        msg.u.a5.ctrl_heat_set.spa_set_point = 0;  // Don't change spa
    } else {
        msg.u.a5.ctrl_heat_set.pool_set_point = 0;  // Don't change pool
        msg.u.a5.ctrl_heat_set.spa_set_point = temp_f;
    }

    if (xQueueSend(pending_commands_q_, &msg, 0) != pdPASS) {
        ESP_LOGW(TAG, "Failed to queue thermostat command");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t
MatterBridge::handle_thermostat_mode_write(uint16_t endpoint_id, uint8_t mode)
{
    int thermo_idx = find_thermostat_index(endpoint_id);
    if (thermo_idx < 0) {
        ESP_LOGW(TAG, "Unknown thermostat endpoint: %u", endpoint_id);
        return ESP_ERR_NOT_FOUND;
    }

    // Matter Thermostat SystemMode: 0=Off, 1=Auto, 3=Cool, 4=Heat
    // We only support Off (0) and Heat (4)
    bool active = (mode == 4);  // Heat mode

    ESP_LOGI(TAG, "Matter thermostat mode write: thermo %d = %s", thermo_idx, active ? "Heat" : "Off");

    // Map thermostat to circuit
    uint8_t circuit_idx;
    if (thermo_idx == MATTER_POOL_THERMO_IDX) {
        circuit_idx = 5;  // network_pool_circuit_t::POOL
    } else {
        circuit_idx = 0;  // network_pool_circuit_t::SPA
    }

    // Create network message to turn circuit on/off
    network_msg_t msg = {};
    msg.typ = network_msg_typ_t::CTRL_CIRCUIT_SET;
    msg.u.a5.ctrl_circuit_set.circuit_plus_1 = circuit_idx + 1;
    msg.u.a5.ctrl_circuit_set.set_value(active);

    if (xQueueSend(pending_commands_q_, &msg, 0) != pdPASS) {
        ESP_LOGW(TAG, "Failed to queue thermostat mode command");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t
MatterBridge::attribute_update_callback(
    esp_matter::attribute::callback_type_t type,
    uint16_t endpoint_id,
    uint32_t cluster_id,
    uint32_t attribute_id,
    esp_matter_attr_val_t * val,
    void * priv_data)
{
    auto * bridge = static_cast<MatterBridge *>(priv_data);
    if (bridge == nullptr) {
        return ESP_OK;  // No bridge context, allow the update
    }

    // We only handle PRE_UPDATE for write validation and command queuing
    if (type != attribute::callback_type_t::PRE_UPDATE) {
        return ESP_OK;
    }

    ESP_LOGD(TAG, "Attribute update: endpoint=%u, cluster=0x%04lx, attr=0x%04lx",
             endpoint_id, static_cast<unsigned long>(cluster_id), static_cast<unsigned long>(attribute_id));

    // Handle OnOff cluster writes (circuit control)
    if (cluster_id == OnOff::Id && attribute_id == OnOff::Attributes::OnOff::Id) {
        return bridge->handle_onoff_write(endpoint_id, val->val.b);
    }

    // Handle Thermostat cluster writes
    if (cluster_id == Thermostat::Id) {
        if (attribute_id == Thermostat::Attributes::OccupiedHeatingSetpoint::Id) {
            return bridge->handle_thermostat_setpoint_write(endpoint_id, val->val.i16);
        }
        if (attribute_id == Thermostat::Attributes::SystemMode::Id) {
            return bridge->handle_thermostat_mode_write(endpoint_id, val->val.u8);
        }
    }

    return ESP_OK;
}

esp_err_t
MatterBridge::identification_callback(
    esp_matter::identification::callback_type_t type,
    uint16_t endpoint_id,
    uint8_t effect_id,
    uint8_t effect_variant,
    void * priv_data)
{
    ESP_LOGI(TAG, "Identification callback: endpoint=%u, effect=%u, variant=%u",
             endpoint_id, effect_id, effect_variant);

    // In a production device, this would trigger visual/audio feedback
    // (e.g., blink LED, beep) to help identify the device during commissioning.
    // For OPNpool, we could potentially trigger a relay briefly or log to console.

    return ESP_OK;
}

/// @}

}  // namespace matter
}  // namespace opnpool
}  // namespace esphome

#endif  // USE_MATTER
