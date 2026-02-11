/**
 * @file opnpool_climate.cpp
 * @brief Actuates climate settings from Home Assistant on the pool controller.
 *
 * @details
 * Implements the climate entity interface for the OPNpool component, allowing ESPHome to
 * control and monitor pool/spa heating via RS-485. Handles mapping between ESPHome
 * climate calls and pool controller commands, and updates climate state from pool
 * controller feedback.
 *
 * ESPHome operates in a single-threaded environment, so explicit thread safety measures
 * are not required within the main task context. The maximum number of climates is
 * limited by the use of uint8_t for indexing.
 *
 * restore_mode is ignored because state is always synchronized from the pool controller.
 * The component will always reflect the actual state of the pool/spa thermostats as 
 * reported by the pool controller.
 * 
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/log.h>
#include <type_traits>

#include "utils/to_str.h"
#include "opnpool_climate.h"  // no other #includes that could make a circular dependency
#include "core/opnpool.h"      // no other #includes that could make a circular dependency
#include "ipc/ipc.h"          // no other #includes that could make a circular dependency
#include "pool_task/network_msg.h"  // #includes datalink_pkt.h, that doesn't #include others that could make a circular dependency
#include "core/poolstate.h"
#include "opnpool_switch.h"
#include "utils/enum_helpers.h"
#include "core/opnpool_ids.h"

#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

constexpr char TAG[] = "opnpool_climate";

/**
 * @brief Converts a temperature from Celsius to Fahrenheit.
 *
 * @param[in] c Temperature in Celsius.
 * @return      Temperature in Fahrenheit.
 */
[[nodiscard]] static float
_celsius_to_fahrenheit(float c) {
    return c * 9.0f / 5.0f + 32.0f;
}

/**
 * @brief Maps a thermostat type to its corresponding pool circuit index.
 *
 * @param[in] thermo_typ The thermostat type (POOL or SPA).
 * @return               The pool circuit index, or 0 if invalid.
 */
[[nodiscard]] static uint8_t
_thermo_typ_to_pool_circuit_idx(poolstate_thermo_typ_t const thermo_typ)
{
    switch (thermo_typ) {
        case poolstate_thermo_typ_t::POOL:
            return enum_index(network_pool_circuit_t::POOL);
        case poolstate_thermo_typ_t::SPA:
            return enum_index(network_pool_circuit_t::SPA);
        default:
            ESP_LOGE(TAG, "Invalid thermo_typ: %d", static_cast<int>(thermo_typ));
            return 0;  // invalid index, but will prevent out-of-bounds access if used
    }
}

/**
 * @brief Dump the configuration and last known state of the climate entity.
 *
 * @details
 * Logs the configuration details for this climate entity, including its ID, last known
 * current and target temperatures, last mode, last custom preset, and last action. This
 * information is useful for diagnostics and debugging, providing visibility into the
 * entity's state and configuration at runtime.
 */

void
OpnPoolClimate::dump_config()
{
    poolstate_thermo_typ_t thermo_typ = get_thermo_typ();

    LOG_CLIMATE("  ", "Climate", this);
    ESP_LOGCONFIG(TAG, "    Thermostat: %s", enum_str(thermo_typ));
    ESP_LOGCONFIG(TAG, "    Last current temp: %.1f°C", last_.valid ? last_.current_temp : -999.0f);
    ESP_LOGCONFIG(TAG, "    Last target temp: %.1f°C", last_.valid ? last_.target_temp : -999.0f);
    ESP_LOGCONFIG(TAG, "    Last mode: %s", last_.valid ? enum_str(last_.mode) : "<unknown>");
    ESP_LOGCONFIG(TAG, "    Last custom preset: %s", last_.valid ? last_.custom_preset : "<unknown>");
    ESP_LOGCONFIG(TAG, "    Last action: %s", last_.valid ? enum_str(last_.action) : "<unknown>");
}


/**
 * @brief Get the climate traits for the OPNpool climate entity
 *
 * @details
 * Defines the supported modes, temperature range, and custom presets for the OPNpool
 * climate entity. Specifies how the pool/spa heating interface appears to Home Assistant,
 * including available climate modes, valid temperature range, and selectable heating
 * sources. Home Assistant will use the custom presets (Heat/SolarPreferred/Solar), but
 * has to use the regular preset (NONE) to disable the heating source.
 *
 * @return The climate traits defining supported modes, presets, and temperature range.
 */
climate::ClimateTraits
OpnPoolClimate::traits()
{
    auto traits = climate::ClimateTraits();  // see climate_traits.h
    
    traits.set_supported_modes({
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_HEAT
    });
    traits.set_visual_min_temperature(0);   // 32°F in Celsius
    traits.set_visual_max_temperature(43);  // 110°F in Celsius
    traits.set_visual_temperature_step(1);

        // support for disabling the heating source
    traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);

        // custom heat sources
    traits.set_supported_custom_presets({
        enum_str(network_heat_src_t::Heat),
        enum_str(network_heat_src_t::SolarPreferred),
        enum_str(network_heat_src_t::Solar)
    });
    return traits;
}


/**
 * @brief
 * Handles requests from Home Assistant to change the climate state of the pool or spa.
 *
 * @details
 * Processes incoming commands for target temperature, climate mode (OFF/HEAT), and heat
 * source (custom preset). The pool controller doesn't have a concept of climate modes.
 * Instead we map it to the pool/spa circuit switch. Constructs and sends protocol
 * messages to the pool controller if thermostat settings have changed. Unsupported
 * climate modes are logged and reported to Home Assistant as OFF. State is not published
 * immediately; updates are sent after confirmation from the pool controller.
 *
 * @param[in] call The climate call object containing requested changes from Home Assistant.
 */
void
OpnPoolClimate::control(const climate::ClimateCall &call)
{
    if (!this->parent_) { ESP_LOGW(TAG, "Parent unknown"); return; }

    poolstate_thermo_typ_t const thermo_typ = get_thermo_typ();
    uint8_t const thermo_idx = enum_index(thermo_typ);

    uint8_t const thermo_pool_idx = enum_index(climate_id_t::POOL_CLIMATE);
    uint8_t const thermo_spa_idx = enum_index(climate_id_t::SPA_CLIMATE);

    PoolState * const state_class_ptr = parent_->get_opnpool_state();
    if (!state_class_ptr) { ESP_LOGW(TAG, "Pool state unknown"); return; }

    poolstate_t state;
    state_class_ptr->get(&state);
    
    datalink_addr_t const controller_addr = state.system.addr.value; // get learned controller address
    if (!controller_addr.is_controller()) {
        ESP_LOGW(TAG, "Controller address still unknown, cannot send control message");
        return;
    }

       // get the state of both thermostats ('cause the resulting network_msg needs to reference both)
    poolstate_thermo_t thermos_old[enum_count<poolstate_thermo_typ_t>()];
    poolstate_thermo_t thermos_new[enum_count<poolstate_thermo_typ_t>()];
    memcpy(thermos_old, state.thermos, sizeof(thermos_old));
    memcpy(thermos_new, state.thermos, sizeof(thermos_new));

        // the resulting network_msg references the heat_src and set_point_in_f value of 
        // the other thermostat. make sure they're valid before proceeding
    if (!thermos_new[thermo_pool_idx].heat_src.valid || !thermos_new[thermo_pool_idx].set_point_in_f.valid || 
        !thermos_new[thermo_spa_idx ].heat_src.valid || !thermos_new[thermo_spa_idx ].set_point_in_f.valid) {
        return; // bail out (user will have to try again later)
    }

        // handle target temperature changes

    if (call.get_target_temperature().has_value()) {

        auto const target_temp_c = *call.get_target_temperature();
        auto const target_temp_f = _celsius_to_fahrenheit(target_temp_c);
        ESP_LOGV(TAG, "HA requests %s to %.1f°F", enum_str(thermo_typ), target_temp_f);

        thermos_new[thermo_idx].set_point_in_f = {
            .valid = true,
            .value = static_cast<uint8_t>(target_temp_f)
        };
    }

        // handle mode changes (OFF, HEAT, AUTO) by turning the POOL or SPA circuit on/off

    if (call.get_mode().has_value()) {

        climate::ClimateMode const requested_mode = *call.get_mode();
        
        ESP_LOGV(TAG, "HA requests %s to mode=%u", enum_str(thermo_typ), static_cast<int>(requested_mode));

        auto circuit_idx = _thermo_typ_to_pool_circuit_idx(thermo_typ);

        switch (requested_mode) {
            case climate::CLIMATE_MODE_OFF:  // mode 0
                ESP_LOGVV(TAG, "Turning off switch[%u]", circuit_idx);
                parent_->get_switch(circuit_idx)->write_state(false);
                break;                
            case climate::CLIMATE_MODE_HEAT:  // mode 3
                ESP_LOGVV(TAG, "Turning on switch[%u]", circuit_idx);
                parent_->get_switch(circuit_idx)->write_state(true);
                break;
            default:
                ESP_LOGW(TAG, "Unsupported requested_mode: %d", static_cast<int>(requested_mode));
                this->mode = climate::CLIMATE_MODE_OFF;
                this->action = climate::CLIMATE_ACTION_OFF;
                this->publish_state();
                break;
        }
    }

        // handle heat source changes (based on custom preset)

    char const * preset_str = call.get_custom_preset().c_str();
    if (*preset_str != '\0') {

        ESP_LOGV(TAG, "HA requests %s to %s", enum_str(thermo_typ), preset_str);

        for (auto heat_src : magic_enum::enum_values<network_heat_src_t>()) {
            if (strcasecmp(preset_str, enum_str(heat_src)) == 0) {
                thermos_new[thermo_idx].heat_src = {
                    .valid = true,
                    .value = heat_src
                };
                break;
            }
        }
        
    } else if (call.get_preset().has_value()) {

        climate::ClimatePreset new_preset = *call.get_preset();
        
        if (new_preset == climate::CLIMATE_PRESET_NONE) {
            ESP_LOGV(TAG, "HA requests %s to NONE", enum_str(thermo_typ));
            thermos_new[thermo_idx].heat_src = {
                .valid = true,
                .value = network_heat_src_t::NONE
            };
        }
    }        

        // actuate thermostat changes if necessary

    bool const thermos_changed = memcmp(thermos_old, thermos_new, sizeof(thermos_old)) != 0;

    if (thermos_changed) {

        network_msg_t msg = {};
        msg.src = datalink_addr_t::remote();
        msg.dst = controller_addr;
        msg.typ = network_msg_typ_t::CTRL_HEAT_SET;
        msg.u.a5.ctrl_heat_set.pool_set_point = thermos_new[thermo_pool_idx].set_point_in_f.value;
        msg.u.a5.ctrl_heat_set.spa_set_point = thermos_new[thermo_spa_idx].set_point_in_f.value;
        msg.u.a5.ctrl_heat_set.heat_src.set_pool(thermos_new[thermo_pool_idx].heat_src.value);
        msg.u.a5.ctrl_heat_set.heat_src.set_spa(thermos_new[thermo_spa_idx].heat_src.value);

        ESP_LOGV(TAG, "Sending HEAT_SET: pool=%u°F, spa=%u°F, heat_src=%u,%u", 
                  msg.u.a5.ctrl_heat_set.pool_set_point, 
                  msg.u.a5.ctrl_heat_set.spa_set_point,
                  (uint8_t)msg.u.a5.ctrl_heat_set.heat_src.get_pool(),
                  (uint8_t)msg.u.a5.ctrl_heat_set.heat_src.get_spa());
                  
        if (ipc_send_network_msg_to_pool_task(&msg, this->parent_->get_ipc()) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send HEAT_SET message to pool task");
        }
    }

    // DON'T publish state here - wait for pool controller confirmation
}


/**
 * @brief
 * Publishes the climate entity state to Home Assistant if any relevant value has changed.
 *
 * @details
 * Compares the new climate state (current temperature, target temperature, mode, custom
 * preset, and action) with the last published state. If any value differs, updates the
 * internal state, sets the appropriate preset or custom preset, and publishes the new
 * state to Home Assistant. This avoids redundant updates to Home Assistant.
 *
 * @param[in] value_current_temperature  The updated current temperature in Celsius.
 * @param[in] value_target_temperature   The updated target temperature in Celsius.
 * @param[in] value_mode                 The updated climate mode (OFF/HEAT).
 * @param[in] value_custom_preset        The updated custom preset string (heat source).
 * @param[in] value_action               The updated climate action (heating, idle, or off).
 */
void 
OpnPoolClimate::publish_value_if_changed(
    float const value_current_temperature, 
    float const value_target_temperature, climate::ClimateMode const value_mode,
    char const * const value_custom_preset, climate::ClimateAction const value_action)
{
    poolstate_thermo_typ_t const thermo_typ = get_thermo_typ();
    last_t * const last = &last_;

    if (!last->valid ||
        last->current_temp != value_current_temperature ||
        last->target_temp != value_target_temperature ||
        last->mode != value_mode ||
        strcasecmp(last->custom_preset, value_custom_preset) != 0 ||
        last->action != value_action) {
        
        this->current_temperature = value_current_temperature;
        this->target_temperature = value_target_temperature;
        this->mode = value_mode;
        this->action = value_action;

            // NONE is handled by the regular preset, not a custom preset
        if (strcasecmp(value_custom_preset, enum_str(network_heat_src_t::NONE)) == 0) {
            ESP_LOGVV(TAG, "Setting thermostat[%s] preset to NONE", enum_str(thermo_typ));
            set_preset_(climate::CLIMATE_PRESET_NONE);
            clear_custom_preset_();
        } else {
            ESP_LOGVV(TAG, "Setting thermostat[%s] custom_preset to %s", enum_str(thermo_typ), value_custom_preset);
            this->set_custom_preset_(value_custom_preset);
        }

        this->publish_state();
            
        *last = {
            .valid = true,
            .current_temp = value_current_temperature,
            .target_temp = value_target_temperature,
            .custom_preset = value_custom_preset,
            .mode = value_mode,
            .action = value_action,
        };
        ESP_LOGVV(TAG, "Published %s: %.0f > %.0f, mode=%s, preset=%s, action=%s", 
            enum_str(thermo_typ),
            value_current_temperature, value_target_temperature,
            enum_str(value_mode), value_custom_preset, enum_str(value_action));
    }
}

}  // namespace opnpool
}  // namespace esphome