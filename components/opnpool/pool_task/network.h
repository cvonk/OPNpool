/**
 * @file network.h
 * @brief Network Layer Definitions for OPNpool component
 *
 * @details
 * This header defines the interface for the network layer of the OPNpool component. The
 * network layer abstracts protocol translation and message construction, enabling
 * reliable communication between the ESP32 and pool controller over RS-485.
 *
 * The network layer provides two main functions:
 * 1. `network_rx_msg()`: overlays a raw datalink packet with a network message structure.
 * 2. `network_create_pkt()`: Creates a datalink packet from a network message.
 *
 * The design supports multiple protocol variants and is intended for use in a
 * single-threaded ESPHome environment. Forward declarations are used to avoid
 * circular dependencies.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2014, 2019, 2022, 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#ifndef __cplusplus
# error "Requires C++ compilation"
#endif

#include <esp_system.h>
#include <esp_types.h>

namespace esphome {
namespace opnpool {

    // forward declarations (to avoid circular dependencies)
struct datalink_pkt_t;
struct network_msg_t;

/**
 * @brief Decode a datalink packet into a network message for higher-level processing.
 *
 * @details
 * Translates a validated datalink packet (from RS-485) into a structured network message,
 * supporting multiple protocol types (A5/CTRL, A5/PUMP, IC/Chlorinator). Determines the
 * message type, populates the network message fields, and sets the transmission opportunity
 * flag if the decoded message allows for a response.
 *
 * @param[in]  pkt           Pointer to the datalink packet to decode.
 * @param[out] msg           Pointer to the network message structure to populate.
 * @param[out] txOpportunity Pointer to a boolean set true if message provides a transmission opportunity.
 * @return                   ESP_OK if the message was successfully decoded, ESP_FAIL otherwise.
 */
[[nodiscard]] esp_err_t network_rx_msg(datalink_pkt_t const * const pkt, network_msg_t * const msg, bool * const txOpportunity);

/**
 * @brief Creates a datalink packet from a network message.
 *
 * @details
 * Allocates a socket buffer, sets protocol headers based on message type, and copies
 * the message payload into the packet data area. The packet is ready for transmission
 * after this function returns successfully.
 *
 * @param[in]  msg Pointer to the network message to convert.
 * @param[out] pkt Pointer to the datalink packet structure to fill.
 * @return         ESP_OK on success, ESP_FAIL if message type is unknown or allocation fails.
 */
[[nodiscard]] esp_err_t network_create_pkt(network_msg_t const * const msg, datalink_pkt_t * const pkt);

}  // namespace opnpool
}  // namespace esphome