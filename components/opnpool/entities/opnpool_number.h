/**
 * @file opnpool_number.h
 * @brief Number entity interface for OPNpool component.
 *
 * @details
 * Declares the OpnPoolNumber class, which implements ESPHome's number interface
 * for setting pool pump speed (RPM) on a Pentair IntelliFlo variable-speed pump.
 *
 * @author bryanribas
 * @copyright Copyright (c) 2026
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/component.h>
#include <esphome/components/number/number.h>

#include "core/opnpool_ids.h"

namespace esphome {
namespace opnpool {

// Forward declarations
struct poolstate_t;
class OpnPool;

/**
 * @brief Number entity for OPNpool component.
 *
 * @details
 * Extends ESPHome's Number and Component classes to provide speed control over
 * a Pentair IntelliFlo variable-speed pump. Sends a three-message command
 * sequence (remote-control enable → VS speed set → run) to the pump via RS-485
 * and publishes the confirmed speed from pump status responses.
 *
 * Valid range: 450 – 3450 RPM (IntelliFlo VS hardware limit).
 */
class OpnPoolNumber : public number::Number, public Component {
  public:
    /**
     * @brief Constructs an OpnPoolNumber entity.
     *
     * @param[in] parent Pointer to the parent OpnPool component.
     * @param[in] id     The number entity ID from number_id_t.
     */
    OpnPoolNumber(OpnPool * parent, uint8_t id)
        : parent_{parent}, id_{static_cast<number_id_t>(id)} {}

    /// @brief Logs configuration details at startup.
    void dump_config();

    /**
     * @brief Publishes the pump speed to Home Assistant if it has changed.
     *
     * @param[in] value The new speed in RPM.
     */
    void publish_value_if_changed(float value);

  protected:
    /**
     * @brief Dispatches a value change from Home Assistant to the matching handler.
     *
     * Routes on the entity id: pump speed (RPM) or chlorinator output level (%).
     *
     * @param[in] value The desired value (RPM or %, depending on the entity).
     */
    void control(float value) override;

    /**
     * @brief Sends the pump speed command sequence (remote-ctrl enable → VS set → run).
     *
     * @param[in] value The desired speed in RPM.
     */
    void control_pump_speed_(float value);

    /**
     * @brief Sends a CHLOR_LEVEL_SET command to the IntelliChlor chlorinator.
     *
     * @param[in] value The desired chlorine output level (0–100 %).
     */
    void control_chlor_level_(float value);

    OpnPool * const   parent_;  ///< Parent OpnPool component.
    number_id_t const id_;      ///< Number entity ID.

    /// @brief Tracks the last published value to avoid redundant updates.
    struct last_t {
        bool  valid;
        float value;
    } last_{false, 0.f};
};

}  // namespace opnpool
}  // namespace esphome
