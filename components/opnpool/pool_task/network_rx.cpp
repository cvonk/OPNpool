/**
 * @file network_rx.cpp
 * @brief Network layer: decode a datalink packet, to form a network message
 * 
 * @details
 * This file implements the decoding logic for the network layer of the OPNpool component.
 * It translates lower-level datalink packets (from RS-485) into higher-level network
 * messages, supporting multiple protocol types (A5/CTRL, A5/PUMP, IC/Chlorinator).
 * 
 * ESPHome operates in a single-threaded environment, so explicit thread safety measures
 * are not required within the pool_task context.
 * 
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2014, 2019, 2022, 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/log.h>

#include "utils/to_str.h"

#include "utils/enum_helpers.h"
#include "datalink.h"
#include "datalink_pkt.h"
#include "network.h"
#include "network_msg.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

constexpr char TAG[] = "network_rx";

/**
 * @brief          Decode an A5_PUMP datalink packet to form a network message.
 *
 * @param[in]  pkt Pointer to the datalink packet to decode.
 * @param[out] msg Pointer to the network message structure to populate.
 * @return         ESP_OK if the message was successfully decoded, ESP_FAIL otherwise.
 */
[[nodiscard]] static esp_err_t
_decode_msg_a5_pump(datalink_pkt_t const * const pkt, network_msg_t * const msg)
{
    bool is_to_pump = pkt->dst.is_pump();

    datalink_pump_typ_t const datalink_pump_typ = pkt->typ.pump;

    network_msg_typ_info_t const * const info = network_msg_typ_get_info(datalink_pump_typ, is_to_pump);
    if (info == nullptr) {
        ESP_LOGW(TAG, "unsupported pump_typ (%s) ", enum_str(datalink_pump_typ));
        return ESP_FAIL;
    }

    if (pkt->data_len != info->size) {

        ESP_LOGW(TAG, "{%s %u} => %s invalid length: expected %lu, got %u", enum_str(datalink_pump_typ), is_to_pump, enum_str(msg->typ), info->size, pkt->data_len);
        return ESP_FAIL;
    }

    msg->typ       = info->network_msg_typ;
    msg->src       = pkt->src;
    msg->dst       = pkt->dst;
    memcpy(msg->u.raw, pkt->data, pkt->data_len);  // saves lots of code to using a union-aware switch

    ESP_LOGVV(TAG, "%s: decoded A5_PUMP msg typ %s", __FUNCTION__, enum_str(msg->typ));
    return ESP_OK;
}


/**
 * @brief         Decode an A5_CTRL datalink packet to form a network message.
 *
 * @param[in]  pkt Pointer to the datalink packet to decode.
 * @param[out] msg Pointer to the network message structure to populate.
 * @return         ESP_OK if the message was successfully decoded, ESP_FAIL otherwise.
 */
[[nodiscard]] static esp_err_t
_decode_msg_a5_ctrl(datalink_pkt_t const * const pkt, network_msg_t * const msg)
{
    datalink_ctrl_typ_t const datalink_ctrl_typ = pkt->typ.ctrl;

    network_msg_typ_info_t const * const info = network_msg_typ_get_info(datalink_ctrl_typ);
    if (info == nullptr) {
        ESP_LOGW(TAG, "unsupported ctrl_typ (%s) ", enum_str(datalink_ctrl_typ));
        return ESP_FAIL;
    }

    if (pkt->data_len != info->size) {
        ESP_LOGW(TAG, "%s => %s invalid length: expected %lu, got %u", enum_str(datalink_ctrl_typ), enum_str(msg->typ), info->size, pkt->data_len);
        return ESP_FAIL;
    }

    msg->typ       = info->network_msg_typ;
    msg->src       = pkt->src;
    msg->dst       = pkt->dst;
    memcpy(msg->u.raw, pkt->data, pkt->data_len);  // saves lots of code to using a union-aware switch

    ESP_LOGVV(TAG, "%s: decoded A5_CTRL msg typ %s", __FUNCTION__, enum_str(msg->typ));
    return ESP_OK;
}


/**
 * @brief         Decode an IC datalink packet to form a network message.
 *
 * @param[in]  pkt Pointer to the datalink packet to decode.
 * @param[out] msg Pointer to the network message structure to populate.
 * @return         ESP_OK if the message was successfully decoded, ESP_FAIL otherwise.
 */
[[nodiscard]] static esp_err_t
_decode_msg_ic_chlor(datalink_pkt_t const * const pkt, network_msg_t * const msg)
{
    datalink_chlor_typ_t const datalink_chlor_typ = pkt->typ.chlor;

    network_msg_typ_info_t const * const info = network_msg_typ_get_info(datalink_chlor_typ);
    if (info == nullptr) {
        ESP_LOGW(TAG, "unsupported chlor_typ (%s) ", enum_str(datalink_chlor_typ));
        return ESP_FAIL;
    }

    if (pkt->data_len != info->size) {
        ESP_LOGW(TAG, "%s => %s invalid length: expected %lu, got %u", enum_str(datalink_chlor_typ), enum_str(msg->typ), info->size, pkt->data_len);
        return ESP_FAIL;
    }

    msg->typ       = info->network_msg_typ;
    msg->src       = pkt->src;
    msg->dst       = pkt->dst;
    memcpy(msg->u.raw, pkt->data, pkt->data_len);  // saves lots of code to using a union-aware switch

    ESP_LOGVV(TAG, "%s: decoded IC msg typ %s", __FUNCTION__, enum_str(msg->typ));
    return ESP_OK;
}


/**
 * @brief Decode a datalink packet into a network message for higher-level processing.
 *
 * This function translates a validated datalink packet (from RS-485) into a structured
 * network message, supporting multiple protocol types (A5/CTRL, A5/PUMP, IC/Chlorinator).
 * It determines the message type, populates the network message fields, and sets the
 * transmission opportunity flag if the decoded message allows for a response.
 *
 * Packets with unsupported or irrelevant destination groups are ignored. The function
 * resets the string conversion mechanism for entity names and logs decoding results for
 * debugging.
 *
 * @param[in]  pkt           Pointer to the datalink packet to decode.
 * @param[out] msg           Pointer to the network message structure to populate.
 * @param[out] txOpportunity Pointer to a boolean set true if message provides a transmission opportunity.
 * @return                   ESP_OK if the message was successfully decoded, ESP_FAIL otherwise.
 */

esp_err_t
network_rx_msg(datalink_pkt_t const * const pkt, network_msg_t * const msg, bool * const txOpportunity)
{
        // reset mechanism that converts various formats to string
    name_reset_idx();

#if 0
        (pkt->prot == datalink_prot_t::IC && dst != datalink_group_addr_t::ALL && dst != datalink_group_addr_t::CHLOR)) {
#endif    

        // silently ignore packets that we don't know how to decode
    datalink_addr_t const dst = pkt->dst;
    if ((pkt->prot == datalink_prot_t::A5_CTRL && dst.is_unknown_90()) ||
        (pkt->prot == datalink_prot_t::IC && !dst.is_broadcast() && !dst.is_chlorinator() )) {

        *txOpportunity = false;
        msg->typ = network_msg_typ_t::IGNORE;
        ESP_LOGV(TAG, "Ignoring packet with prot %s and dst addr %u", enum_str(pkt->prot), dst.addr);
        return ESP_OK;
    }

    esp_err_t result = ESP_FAIL;

    switch (pkt->prot) {
        case datalink_prot_t::A5_CTRL:
            result = _decode_msg_a5_ctrl(pkt, msg);
            break;
        case datalink_prot_t::A5_PUMP:
            result = _decode_msg_a5_pump(pkt, msg);
            break;
        case datalink_prot_t::IC:
            result = _decode_msg_ic_chlor(pkt, msg);
            break;
        default:
            ESP_LOGW(TAG, "unknown prot %u", enum_index(pkt->prot));
    }
    ESP_LOGV(TAG, "Decoded pkt (prot=%s dst=%u) to %s", enum_str(pkt->prot), dst.addr, enum_str(msg->typ));

    *txOpportunity =
        pkt->prot == datalink_prot_t::A5_CTRL &&
        pkt->src.is_controller() &&
        pkt->dst.is_broadcast();

    return result;
}

} // namespace opnpool
} // namespace esphome