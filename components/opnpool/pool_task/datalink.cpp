/**
 * @file datalink.cpp
 * @brief Data Link layer: bytes from the RS485 transceiver to/from data packets
 *
 * @details
 * This file implements core functions for the OPNpool data link layer, facilitating the
 * conversion between raw RS485 byte streams and structured protocol data packets. It
 * provides utilities for handling protocol preambles and postambles, address group
 * extraction and composition, and checksum calculation for packet integrity. These
 * foundational routines are used by both the transmitter and receiver to ensure reliable
 * communication between the ESPHome component and pool equipment over the RS485 bus.
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

#include "datalink.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

datalink_preamble_a5_t datalink_preamble_a5  = { 0x00, 0xFF, 0xA5 };  // use of 0xA5 in the preamble makes the detection more reliable
datalink_preamble_ic_t datalink_preamble_ic  = { 0x10, 0x02 };
datalink_preamble_ic_t datalink_postamble_ic = { 0x10, 0x03 };

#if 0
/**
 * @brief               Composes a device address from an address group and device ID.
 *
 * @param[in] group     The address group (high nibble).
 * @param[in] device_id The device ID within the group (low nibble).
 * @return              The composed 8-bit device address.
 */
datalink_addr_t
datalink_addr(datalink_group_addr_t const group, datalink_pump_id_t const device_id)
{
    datalink_addr_t addr = {};
    addr.set_group_addr(group);
    addr.set_pump_id(device_id);

    return addr;
}
#endif

/**
 * @brief           Calculates the checksum for a data buffer.
 *
 * @param[in] start Pointer to the start of the data buffer.
 * @param[in] stop  Pointer to one past the end of the data buffer (exclusive).
 * @return          The calculated 16-bit checksum (sum of all bytes).
 */
uint16_t
datalink_calc_checksum(uint8_t const * const start, uint8_t const * const stop)
{
    uint16_t checksum = 0;
    for (uint8_t const * byte = start; byte < stop; byte++) {
        checksum += *byte;
    }
    return checksum;
}

} // namespace opnpool
} // namespace esphome