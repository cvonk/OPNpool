/**
 * @file opnpool_select.cpp
 * @brief Actuates light color mode from Home Assistant on the pool controller.
 *
 * @details
 * Implements the select entity interface for controlling Pentair IntelliBrite
 * light color/show modes over RS-485. Sends a LIGHT_COLOR_SET (0x60) command
 * with a 2-byte payload [color_byte, 0x00] to the EasyTouch controller when
 * a mode is selected in Home Assistant.
 *
 * @author bryanribas
 * @copyright Copyright (c) 2026
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esphome/core/log.h>

#include "opnpool_select.h"
#include "core/opnpool.h"
#include "core/poolstate.h"
#include "ipc/ipc.h"
#include "pool_task/datalink.h"
#include "pool_task/network_msg.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

constexpr char TAG[] = "opnpool_select";

/// @brief Maps option name strings to their EasyTouch color byte values.
struct color_entry_t {
    const char * name;
    uint8_t      value;
};

static constexpr color_entry_t COLOR_TABLE[] = {
    {"Party",     0xB1},
    {"Romance",   0xB2},
    {"Caribbean", 0xB3},
    {"American",  0xB4},
    {"Sunset",    0xB5},
    {"Royal",     0xB6},
    {"Blue",      0xC1},
    {"Green",     0xC2},
    {"Red",       0xC3},
    {"White",     0xC4},
    {"Magenta",   0xC5},
};

/**
 * @brief Logs configuration details at startup.
 */
void
OpnPoolSelect::dump_config()
{
    LOG_SELECT("  ", "Select", this);
    ESP_LOGCONFIG(TAG, "    ID: %u", static_cast<uint8_t>(id_));
}

/**
 * @brief Sends a LIGHT_COLOR_SET command when the user picks a color mode.
 *
 * @param[in] value The selected option string (e.g. "Blue", "Party").
 */
void
OpnPoolSelect::control(const std::string &value)
{
    if (!this->parent_) { ESP_LOGW(TAG, "Parent unknown"); return; }

    PoolState * const state_class_ptr = parent_->get_opnpool_state();
    if (!state_class_ptr) { ESP_LOGW(TAG, "Pool state unknown"); return; }

    poolstate_t state;
    state_class_ptr->get(&state);

    datalink_addr_t const controller_addr = state.system.addr.value;
    if (!controller_addr.is_controller()) {
        ESP_LOGW(TAG, "Controller address still unknown, cannot send light color command");
        return;
    }

    // look up color byte for the selected option name
    uint8_t color_byte = 0;
    bool found = false;
    for (auto const &entry : COLOR_TABLE) {
        if (value == entry.name) {
            color_byte = entry.value;
            found = true;
            break;
        }
    }
    if (!found) {
        ESP_LOGW(TAG, "Unknown light color option: %s", value.c_str());
        return;
    }

    network_msg_t msg;
    msg.src = datalink_addr_t::wireless_remote();  // 0x22
    msg.dst = controller_addr;
    msg.typ = network_msg_typ_t::CTRL_LIGHT_COLOR_SET;
    msg.u.a5.ctrl_light_color_set = {
        .color   = color_byte,
        .padding = 0x00
    };

    ESP_LOGV(TAG, "Sending LIGHT_COLOR_SET: %s (0x%02X) dst=0x%02X", value.c_str(), color_byte, controller_addr.addr);
    if (ipc_send_network_msg_to_pool_task(&msg, this->parent_->get_ipc()) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send LIGHT_COLOR_SET message to pool task");
        return;
    }

    this->publish_state(value);
}

}  // namespace opnpool
}  // namespace esphome
