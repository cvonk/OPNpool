/**
 * @file opnpool_climate.h
 * @brief Climate entity interface for OPNpool component.
 *
 * @details
 * Declares the OpnPoolClimate class, which implements ESPHome's climate interface
 * for controlling and monitoring pool/spa heating via RS-485. Supports temperature
 * control, heat source selection, and mode changes through Home Assistant.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/component.h>
#include <esphome/components/climate/climate.h>

#include "core/opnpool_ids.h"

namespace esphome {
namespace opnpool {

    // forward declarations (to avoid circular dependencies)
struct poolstate_t;
class OpnPool;

/**
 * @brief Climate entity for OPNpool component.
 *
 * @details
 * Extends ESPHome's Climate and Component classes to provide pool/spa thermostat
 * control. Maps climate modes to pool circuit switches and custom presets to heat
 * sources. Only publishes state updates when values change.
 */
class OpnPoolClimate : public climate::Climate, public Component {

  public:
    /**
     * @brief Constructs an OpnPoolClimate instance.
     *
     * @param[in] parent Pointer to the parent OpnPool component.
     * @param[in] id     The climate entity ID (POOL or SPA).
     */
    OpnPoolClimate(OpnPool* parent, uint8_t id) : parent_{parent}, id_{static_cast<climate_id_t>(id)} {}

    /**
     * @brief Dumps the configuration and last known state of the climate entity.
     *
     * Called by ESPHome during startup. Set logger for this module to INFO or
     * higher to see output.
     */
    void dump_config();

    /**
     * @brief Gets the climate traits for this entity.
     *
     * @return The climate traits defining supported modes, presets, and temperature range.
     */
    climate::ClimateTraits traits() override;

    /**
     * @brief Handles climate control requests from Home Assistant.
     *
     * @param[in] call The climate call object containing requested changes.
     */
    void control(const climate::ClimateCall &call) override;

    /**
     * @brief Gets the thermostat type for this climate entity.
     *
     * @return The thermostat type (POOL or SPA).
     */
    poolstate_thermo_typ_t get_thermo_typ() const { return this->thermo_typ_; }

    /**
     * @brief Publishes the climate state to Home Assistant if any value has changed.
     *
     * @param[in] value_current_temperature The updated current temperature in Celsius.
     * @param[in] value_target_temperature  The updated target temperature in Celsius.
     * @param[in] value_mode                The updated climate mode (OFF/HEAT).
     * @param[in] value_custom_preset       The updated custom preset string (heat source).
     * @param[in] value_action              The updated climate action (heating, idle, or off).
     */
    void publish_value_if_changed(
        float const value_current_temperature,
        float const value_target_temperature, climate::ClimateMode const value_mode,
        char const * value_custom_preset, climate::ClimateAction const value_action
    );

  protected:
    OpnPool * const              parent_;      ///< Parent OpnPool component.
    climate_id_t const           id_;          ///< Climate entity ID.
    poolstate_thermo_typ_t const thermo_typ_ = climate_id_to_poolstate_thermo(id_);  ///< Thermostat type (POOL/SPA).

    /// @brief Tracks the last published state to avoid redundant updates.
    struct last_t {
        bool                    valid;          ///< True if a value has been published at least once.
        float                   current_temp;   ///< Last published current temperature.
        float                   target_temp;    ///< Last published target temperature.
        char const *            custom_preset;  ///< Last published custom preset string.
        climate::ClimateMode    mode;           ///< Last published climate mode.
        climate::ClimateAction  action;         ///< Last published climate action.
    } last_ = {
        .valid = false,
        .current_temp = 0.0f,
        .target_temp = 0.0f,
        .custom_preset = nullptr,
        .mode = climate::CLIMATE_MODE_OFF,
        .action = climate::CLIMATE_ACTION_OFF
    };
};

}  // namespace opnpool
}  // namespace esphome