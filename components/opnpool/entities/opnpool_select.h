/**
 * @file opnpool_select.h
 * @brief Select entity interface for OPNpool component.
 *
 * @details
 * Declares the OpnPoolSelect class, which implements ESPHome's select interface
 * for choosing the IntelliBrite light color/show mode on a Pentair EasyTouch
 * controller. Sends a 0x60 (LIGHT_COLOR_SET) command to the controller when
 * the user picks a mode in Home Assistant.
 *
 * @author bryanribas
 * @copyright Copyright (c) 2026
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/component.h>
#include <esphome/components/select/select.h>

#include "core/opnpool_ids.h"

namespace esphome {
namespace opnpool {

// Forward declarations
class OpnPool;

/**
 * @brief Select entity for OPNpool light color mode.
 *
 * @details
 * Extends ESPHome's Select and Component classes to control IntelliBrite
 * light color/show modes via the Pentair EasyTouch RS-485 protocol. When
 * a color mode is selected in Home Assistant, a LIGHT_COLOR_SET (0x60)
 * command is sent to the controller with the corresponding color byte.
 */
class OpnPoolSelect : public select::Select, public Component {
  public:
    /**
     * @brief Constructs an OpnPoolSelect entity.
     *
     * @param[in] parent Pointer to the parent OpnPool component.
     * @param[in] id     The select entity ID from select_id_t.
     */
    OpnPoolSelect(OpnPool * parent, uint8_t id)
        : parent_{parent}, id_{static_cast<select_id_t>(id)} {}

    /// @brief Logs configuration details at startup.
    void dump_config();

  protected:
    /**
     * @brief Handles color mode selection from Home Assistant.
     *
     * Sends CTRL_LIGHT_COLOR_SET to the pool controller with the
     * color byte corresponding to the selected option name.
     *
     * @param[in] value The selected option string (e.g. "Blue", "Party").
     */
    void control(const std::string &value) override;

    OpnPool * const   parent_;  ///< Parent OpnPool component.
    select_id_t const id_;      ///< Select entity ID.
};

}  // namespace opnpool
}  // namespace esphome
