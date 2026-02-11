/**
 * @file datalink_pkt.h
 * @brief Defines the core data structure for the OPNpool data link layer.
 *
 * @details
 * This header defines the core data structures, enums, and utility functions for the
 * OPNpool data link layer. The file defines the main datalink packet structure used for
 * encapsulating protocol messages, along with macros for alignment and packing to ensure
 * compatibility with embedded hardware.
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

#include "datalink.h"

#ifndef PACK8
# define PACK8 __attribute__((aligned( __alignof__(uint8_t)), packed))
#endif

namespace esphome {
namespace opnpool {

    // forward declarations (to avoid circular dependencies)
struct skb_t;
using skb_handle_t = skb_t *;

/**
 * @brief Enumerates the supported data link layer protocol types in the Pentair protocol.
 *
 * @details
 * This enum class defines the protocol identifiers used at the data link layer
 * for communication between the ESPHome component and pool equipment. It
 * distinguishes between the IC protocol, A5 controller protocol, and A5 pump protocol.
 */
enum class datalink_prot_t : uint8_t {
    IC      = 0x00,  ///< Protocol type for the IntelliCenter (IC) protocol (e.g. Chlorinator).
    A5_CTRL = 0x01,  ///< Protocol type for the A5 controller protocol.
    A5_PUMP = 0x02,  ///< Protocol type for the A5 pump protocol.
    NONE    = 0xFF   ///< No valid protocol was detected.
};

/**
 * @brief Controller message types
 *
 * @details
 * This enum class defines the message types for the A5 controller protocol.
 * Each value corresponds to a specific request or response type used in the A5
 * controller protocol.
 * 
 * @note Bit 6-7 (0xC0) indicate Request (0xC0), Response (0x40), or Set (0x00).
 */
enum class datalink_ctrl_typ_t : uint8_t {
    SET_ACK          = 0x01,
    STATE_BCAST      = 0x02,
    CANCEL_DELAY     = 0x03,
    TIME_RESP        = 0x05,
    TIME_SET         = 0x85,
    TIME_REQ         = 0xC5,
    CIRCUIT_RESP     = 0x06,
    CIRCUIT_SET      = 0x86,
    CIRCUIT_REQ      = 0xC6,
    HEAT_RESP        = 0x08,
    HEAT_SET         = 0x88,
    HEAT_REQ         = 0xC8,
    HEAT_PUMP_RESP   = 0x10,
    HEAT_PUMP_SET    = 0x90,
    HEAT_PUMP_REQ    = 0xD0,
    SCHED_RESP       = 0x1E,
    SCHED_SET        = 0x9E,
    SCHED_REQ        = 0xDE,
    LAYOUT_RESP      = 0x21,
    LAYOUT_SET       = 0xA1,
    LAYOUT_REQ       = 0xE1,
    CUSTOM_MODEL_REQ = 0xCA,
    CIRC_NAMES_RESP  = 0x0B,
    CIRC_NAMES_REQ   = 0xCB,
    SCHEDS_RESP      = 0x11,
    SCHEDS_REQ       = 0xD1,
    CHEM_RESP        = 0x12,
    CHEM_REQ         = 0xD2,
    VALVE_RESP       = 0x1D,
    VALVE_REQ        = 0xDD,
    SOLARPUMP_RESP   = 0x22,
    SOLARPUMP_REQ    = 0xE2,
    DELAY_RESP       = 0x23,
    DELAY_REQ        = 0xE3,
    HEAT_SETPT_RESP  = 0x28,
    HEAT_SETPT_REQ   = 0xE8,
    VERSION_RESP     = 0xFC,
    VERSION_REQ      = 0xFD
    // SPA_CTRL_REQ     = 0xD6,
    // SPA_CTRL_RESP    = 0x16,
    // INTELLICHOR_REQ  = 0xD9,
    // INTELLICHOR_RESP = 0x19,
    // HS_VALVE_REQ     = 0xDE,
    // HS_VALVE_RESP    = 0x1E,
    // IS4_IS10_REQ     = 0xE0
    // IS4_IS10_RESP    = 0x20,
    // SPA_REMOTE_REQ   = 0xE1
    // SPA_REMOTE_RESP  = 0x21,
    // LIGHT_REQ        = 0xE7,
    // LIGHT_RESP       = 0x27,
};

/**
 * @brief Pump message types
 *
 * @details
 * This enum class defines the message types for the A5 pump protocol.
 * Each value corresponds to a specific request or response type used in the A5
 * pump protocol.
 */
enum class datalink_pump_typ_t : uint8_t {
    REG         = 0x01,
    REMOTE_CTRL = 0x04,
    RUN_MODE    = 0x05,  // intellicom uses this
    RUN         = 0x06,  // naming it POWER would conflict with pump power measurement
    STATUS      = 0x07,
    REG_VF      = 0x09,  // variable flow rate (gal/min)
    REG_VS      = 0x0A,  // variable speed (RPM)
    REJECTING   = 0xFF
};

/**
 * @brief Chlorinator message types
 * 
 * @details
 * This enum class defines the message types for the IntelliCenter (IC) chlorinator
 * protocol. Each value corresponds to a specific request or response type used in the IC
 * protocol.
 *
 * @note  The enum value count MUST MATCH datalink_chlor_typ_sizes[] in
 *        datalink_rx.cpp. A compile-time assertion validates the count.
 */
enum class datalink_chlor_typ_t : uint8_t {
    CONTROL_REQ  = 0x00,  ///< Control request message
    CONTROL_RESP = 0x01,  ///< Control response message
    UNKNOWN_02   = 0x02,
    MODEL_RESP   = 0x03,  ///< Name response message (only when chlor is active)
    UNKNOWN_04   = 0x04,
    UNKNOWN_05   = 0x05,
    UNKNOWN_06   = 0x06,
    UNKNOWN_07   = 0x07,
    UNKNOWN_08   = 0x08,
    UNKNOWN_09   = 0x09,
    UNKNOWN_0A   = 0x0A,
    UNKNOWN_0B   = 0x0B,
    UNKNOWN_0C   = 0x0C,
    UNKNOWN_0D   = 0x0D,
    UNKNOWN_0E   = 0x0E,
    UNKNOWN_0F   = 0x0F,
    UNKNOWN_10   = 0x10,
    LEVEL_SET    = 0x11,  ///< Level set message [%]
    LEVEL_RESP   = 0x12,  ///< Level response message (to LEVEL_SET or LEVEL_SET10)
    ICHLOR_PING  = 0x13,  ///< maybe a keep-alive? has no payload.
    MODEL_REQ    = 0x14,  ///< Name request message
    LEVEL_SET10  = 0x15,  ///< Level set message percentage with one decimal place [%*10]
    ICHLOR_BCAST = 0x16   ///< iChlor status message  (level and temp on IC30)
};

/**
 * @brief Union representing the possible message type fields in a data link layer packet.
 *
 * @details
 * This union allows access to the message type as a controller, pump, or chlorinator
 * message type, depending on the protocol context. It also provides access to the
 * raw 8-bit value for generic handling.
 */
union datalink_typ_t {
    datalink_ctrl_typ_t  ctrl;  ///< Controller message type (datalink_ctrl_typ_t).
    datalink_pump_typ_t  pump;  ///< Pump message type (datalink_pump_typ_t).
    datalink_chlor_typ_t chlor; ///< Chlorinator message type (datalink_chlor_typ_t).
    uint8_t              raw;   ///< Raw 8-bit value for generic or protocol-agnostic access.
};

/**
 * @brief Represents a single byte of data in the data link layer protocol.
 *
 * @details
 * This type is used for the payload buffer in data link layer packets.
 * It is a simple abstraction of a byte.
 */
using datalink_data_t = uint8_t;

/**
 * @brief Represents a data link layer packet in the Pentair protocol.
 *
 * @details
 * Encapsulates all metadata and payload required for a protocol message,
 * including protocol type, message type (controller, pump, or chlorinator),
 * source and destination addresses, a pointer to the data payload, payload
 * length, and a handle to the associated socket buffer. This structure
 * is used throughout the data link layer for both receiving and transmitting
 * packets.
 */
struct datalink_pkt_t {
    datalink_prot_t    prot;      ///< Protocol type as detected by `_read_head()`.
    datalink_typ_t     typ;       ///< Message type from datalink_hdr_a5->typ.
    datalink_addr_t    src;       ///< Source address from datalink_hdr_a5->src.
    datalink_addr_t    dst;       ///< Destination address from datalink_hdr_a5->dst.
    datalink_data_t *  data;      ///< Pointer to the data payload buffer.
    size_t             data_len;  ///< Length of the data payload.
    skb_handle_t       skb;       ///< Handle to the socket buffer containing the packet data.
};

} // namespace opnpool
} // namespace esphome