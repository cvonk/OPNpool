/**
 * @file opnpool_number.cpp
 * @brief Actuates pump speed from Home Assistant on the IntelliFlo VS pump.
 *
 * @details
 * Implements the number entity interface for controlling a Pentair IntelliFlo
 * variable-speed pump over RS-485.  When Home Assistant sets a new speed value
 * the following three-message sequence is queued to the pool_task:
 *
 *   1. PUMP_REMOTE_CTRL_SET (0xFF) – enables external (remote) control mode.
 *   2. PUMP_REG_SET – writes the requested RPM to register 0x02C4.
 *   3. PUMP_RUN_SET (0x0A) – commands the pump to start/keep running.
 *
 * State updates flow in the opposite direction: the pool_task receives periodic
 * PUMP_STATUS_RESP packets and the confirmed pump speed is published back to
 * Home Assistant via publish_value_if_changed().
 *
 * @author bryanribas
 * @copyright Copyright (c) 2026
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esphome/core/log.h>

#include "opnpool_number.h"
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

constexpr char TAG[] = "opnpool_number";

/**
 * @brief Logs configuration and last known state of the number entity.
 */
void
OpnPoolNumber::dump_config()
{
    LOG_NUMBER("  ", "Number", this);
    ESP_LOGCONFIG(TAG, "    ID: %u  Last value: %s",
                  static_cast<uint8_t>(id_),
                  last_.valid ? std::to_string(static_cast<int>(last_.value)).c_str() : "Unknown");
}

/**
 * @brief Dispatches a Home Assistant value change to the matching handler.
 *
 * @param[in] value Desired value (RPM for the pump speed entity, % for the chlorinator).
 */
void
OpnPoolNumber::control(float value)
{
    if (!this->parent_) { ESP_LOGW(TAG, "Parent unknown"); return; }

    switch (id_) {
        case number_id_t::PRIMARY_PUMP_SPEED_SETPOINT:
            this->control_pump_speed_(value);
            break;
        case number_id_t::CHLORINATOR_SETPOINT:
            this->control_chlor_level_(value);
            break;
        default:
            ESP_LOGW(TAG, "Unhandled number id %u", static_cast<uint8_t>(id_));
            break;
    }
    // DON'T publish state here — wait for confirmation from the bus.
}

/**
 * @brief Sends the pump speed command sequence when Home Assistant changes the value.
 *
 * @param[in] value Desired pump speed in RPM (450 – 3450).
 */
void
OpnPoolNumber::control_pump_speed_(float value)
{
    PoolState * const state_class_ptr = parent_->get_opnpool_state();
    if (!state_class_ptr) { ESP_LOGW(TAG, "Pool state unknown"); return; }

    poolstate_t state;
    state_class_ptr->get(&state);

    datalink_addr_t const controller_addr = state.system.addr.value;
    if (!controller_addr.is_controller()) {
        ESP_LOGW(TAG, "Controller address still unknown, cannot send pump command");
        return;
    }

    datalink_addr_t const pump_addr = datalink_addr_t::pump(datalink_pump_id_t::PRIMARY);
    uint16_t        const rpm       = static_cast<uint16_t>(value);

    // 1. Enable remote (external) control on the pump.
    {
        network_msg_t msg;
        msg.src                  = controller_addr;
        msg.dst                  = pump_addr;
        msg.typ                  = network_msg_typ_t::PUMP_REMOTE_CTRL_SET;
        msg.u.a5.pump_ctrl.raw   = 0xFF;
        ipc_send_network_msg_to_pool_task(&msg, parent_->get_ipc());
    }

    // 2. Write the requested RPM to the pump's RPM register (0x02C4) via a
    //    "set register" command (action REG, 0x01): payload {0x02, 0xC4, hi, lo}.
    {
        constexpr uint16_t rpm_reg = static_cast<uint16_t>(network_pump_reg_addr_t::RPM);

        network_msg_t msg;
        msg.src                        = controller_addr;
        msg.dst                        = pump_addr;
        msg.typ                        = network_msg_typ_t::PUMP_REG_SET;
        msg.u.a5.pump_reg_set.address  = {
            .high = static_cast<uint8_t>(rpm_reg >> 8),
            .low  = static_cast<uint8_t>(rpm_reg & 0xFF)
        };
        msg.u.a5.pump_reg_set.value    = {
            .high = static_cast<uint8_t>(rpm >> 8),
            .low  = static_cast<uint8_t>(rpm & 0xFF)
        };
        ipc_send_network_msg_to_pool_task(&msg, parent_->get_ipc());
    }

    // 3. Command the pump to run.
    {
        network_msg_t msg;
        msg.src                    = controller_addr;
        msg.dst                    = pump_addr;
        msg.typ                    = network_msg_typ_t::PUMP_RUN_SET;
        msg.u.a5.pump_running.raw  = 0x0A;  // 0x0A = running
        ipc_send_network_msg_to_pool_task(&msg, parent_->get_ipc());
    }

    ESP_LOGV(TAG, "Sent pump speed command: %u RPM", rpm);
    // DON'T publish state here — wait for PUMP_STATUS_RESP confirmation.
}

/**
 * @brief Sends a CHLOR_LEVEL_SET command to the IntelliChlor when Home Assistant changes the value.
 *
 * @details
 * The IC protocol is addressed to the chlorinator (0x50); unlike the A5 control messages it
 * does not require the learned controller address. The confirmed level is published back to
 * Home Assistant from poolstate.chlor.level (updated by the echoed/broadcast CHLOR_LEVEL_SET).
 *
 * @param[in] value Desired chlorine output level (0–100 %).
 */
void
OpnPoolNumber::control_chlor_level_(float value)
{
    if (value < 0.f)   value = 0.f;
    if (value > 100.f) value = 100.f;

    network_msg_t msg;
    msg.src = datalink_addr_t::wireless_remote();  // IC frames carry no src; set for consistency
    msg.dst = datalink_addr_t::chlorinator();      // 0x50
    msg.typ = network_msg_typ_t::CHLOR_LEVEL_SET;
    msg.u.ic.chlor_level_set = {
        .level = static_cast<uint8_t>(value)
    };

    ESP_LOGV(TAG, "Sending CHLOR_LEVEL_SET: %u%%", msg.u.ic.chlor_level_set.level);
    if (ipc_send_network_msg_to_pool_task(&msg, this->parent_->get_ipc()) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send CHLOR_LEVEL_SET message to pool task");
    }
    // DON'T publish state here — wait for the chlorinator level confirmation on the bus.
}

/**
 * @brief Publishes the pump speed to Home Assistant if it has changed.
 *
 * @param[in] value New pump speed in RPM, as reported by the pump.
 */
void
OpnPoolNumber::publish_value_if_changed(float value)
{
    if (!last_.valid || last_.value != value) {
        this->publish_state(value);
        last_ = {.valid = true, .value = value};
        ESP_LOGV(TAG, "Published pump speed: %.0f RPM", value);
    }
}

}  // namespace opnpool
}  // namespace esphome
