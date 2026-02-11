/**
 * @file datalink.h
 * @brief Data Link Layer Definitions for OPNpool component
 *
 * @details
 * This header defines the data structures, enumerations, and function prototypes for the
 * data link layer of the OPNpool component. The data link layer is responsible for
 * framing, parsing, and validating packets exchanged between the ESP32 and the Pentair
 * pool controller over RS-485. It enables reliable communication and protocol abstraction
 * for higher-level network and application logic.
 * 
 * The data link layer provides two functions:
 * 1. `datalink_rx_pkt()`: removes the header and tail of a RS-485 byte stream, verifies
 *    its integrity.
 * 2. `datalink_tx_pkt_queue()`: adds the header and tail to create a RS-485 byte stream.
 *
 * The design supports multiple protocol variants (A5, IC) and hardware configurations,
 * and is intended for use in a single-threaded ESPHome environment.
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

#ifndef PACK8
# define PACK8 __attribute__((aligned( __alignof__(uint8_t)), packed))
#endif

    // forward declarations (to avoid circular dependencies)
struct datalink_pkt_t;
struct rs485_instance_t;
using rs485_handle_t = rs485_instance_t *;

    // common pump ids 
enum class datalink_pump_id_t : uint8_t {
    PRIMARY = 0x00,
    SOLAR   = 0x01
};

using datalink_preamble_a5_t  = uint8_t[3];
using datalink_preamble_ic_t  = uint8_t[2];
using datalink_postamble_ic_t = uint8_t[2];

/**
 * @brief 8-bit device address combining group and device ID.
 *
 * The address byte encodes both the group (high nibble) and device ID (low nibble).
 * This struct provides accessor methods for extracting and setting each component.
 */
struct datalink_addr_t {
    uint8_t addr;  ///< Full address byte: high nibble = group, low nibble = device ID.

        // constants for common addresses
    static constexpr uint8_t ALL                  = 0x00;
    static constexpr uint8_t SUNTOUCH_CONTROLLER  = 0x10;
    static constexpr uint8_t EASYTOUCH_CONTROLLER = 0x20;
    static constexpr uint8_t REMOTE               = 0x21;
    static constexpr uint8_t WIRELESS_REMOTE      = 0x22;  ///< Screen Logic or app
    static constexpr uint8_t QUICKTOUCH_REMOTE    = 0x48;
    static constexpr uint8_t CHLORINATOR          = 0x50;
    static constexpr uint8_t PUMP_BASE            = 0x60;  ///< base value for pump addresses
    static constexpr uint8_t PUMP_ID_MASK         = 0x0F;  ///< mask to extract pump ID (low nibble)
    static constexpr uint8_t BROADCAST            = 0x0F;  ///< broadcast address
    static constexpr uint8_t UNKNOWN_90           = 0x90;

        // factory methods
    static constexpr datalink_addr_t unknown()              { return datalink_addr_t{ALL}; }
    static constexpr datalink_addr_t suntouch_controller()  { return datalink_addr_t{SUNTOUCH_CONTROLLER}; }
    static constexpr datalink_addr_t easytouch_controller() { return datalink_addr_t{EASYTOUCH_CONTROLLER}; }
    static constexpr datalink_addr_t remote()               { return datalink_addr_t{REMOTE}; }
    static constexpr datalink_addr_t wireless_remote()      { return datalink_addr_t{WIRELESS_REMOTE}; }
    static constexpr datalink_addr_t quicktouch_remote()    { return datalink_addr_t{QUICKTOUCH_REMOTE}; }

    static constexpr datalink_addr_t pump(datalink_pump_id_t const pump_id) { 
        return datalink_addr_t{ static_cast<uint8_t>(PUMP_BASE | (static_cast<uint8_t>(pump_id) & PUMP_ID_MASK)) };
    }

        // accessors
    constexpr bool is_controller()  const { return (addr == SUNTOUCH_CONTROLLER || addr == EASYTOUCH_CONTROLLER); }
    constexpr bool is_remote()      const { return (addr == REMOTE || addr == WIRELESS_REMOTE || addr == QUICKTOUCH_REMOTE); }
    constexpr bool is_pump()        const { return (addr & 0xF0) == PUMP_BASE; }
    constexpr bool is_unknown_90()  const { return addr == UNKNOWN_90; }
    constexpr bool is_chlorinator() const { return addr == CHLORINATOR; } 
    constexpr bool is_broadcast()   const { return addr == BROADCAST; }
    constexpr char const * to_str() const { return addr == SUNTOUCH_CONTROLLER  ? "Suntouch" :
                                                   addr == EASYTOUCH_CONTROLLER ? "EasyTouch" : "unknown"; }

    constexpr datalink_pump_id_t get_pump_id() const {
        return static_cast<datalink_pump_id_t>(addr & PUMP_ID_MASK);
    }
} PACK8;
static_assert(sizeof(datalink_addr_t) == 1, "datalink_addr_t must be 1 byte");

/// @brief IC protocol header structure.
struct datalink_hdr_ic_t {
    datalink_addr_t dst;  ///< Destination address.
    uint8_t         typ;  ///< Message type.
} PACK8;

/// @brief A5 protocol header structure.
struct datalink_hdr_a5_t {
    uint8_t         ver;  ///< Protocol version ID.
    datalink_addr_t dst;  ///< Destination address.
    datalink_addr_t src;  ///< Source address.
    uint8_t         typ;  ///< Message type.
    uint8_t         len;  ///< Number of data bytes following.
} PACK8;

/// @brief Union of IC and A5 protocol headers.
union datalink_hdr_t {
    datalink_hdr_ic_t ic;  ///< IC protocol header.
    datalink_hdr_a5_t a5;  ///< A5 protocol header.
} PACK8;

/// @brief A5 protocol head (preamble + header).
struct datalink_head_a5_t {
    uint8_t                ff;        ///< Leading 0xFF byte.
    datalink_preamble_a5_t preamble;  ///< A5 preamble bytes.
    datalink_hdr_a5_t      hdr;       ///< A5 header.
} PACK8;

/// @brief IC protocol head (preamble + header).
struct datalink_head_ic_t {
    uint8_t                ff;        ///< Leading 0xFF byte.
    datalink_preamble_ic_t preamble;  ///< IC preamble bytes.
    datalink_hdr_ic_t      hdr;       ///< IC header.
} PACK8;

/**
 * @brief Data link head union for protocol abstraction.
 *
 * This union provides access to the head fields for both IC and A5 protocol variants. It
 * enables unified handling of protocol-specific head data, including preamble and header,
 * for flexible packet parsing and construction.
 */
union datalink_head_t {
    datalink_head_ic_t ic;  ///< Head structure for IC protocol packets (includes preamble and header).
    datalink_head_a5_t a5;  ///< Head structure for A5 protocol packets (includes preamble and header).
};

uint8_t const DATALINK_MAX_HEAD_SIZE = sizeof(datalink_head_t);

/// @brief A5 protocol tail (checksum).
struct datalink_tail_a5_t {
    uint8_t  checksum[2];  ///< 16-bit checksum (big-endian).
} PACK8;

/// @brief IC protocol tail (checksum + postamble).
struct datalink_tail_ic_t {
    uint8_t                 checksum[1];   ///< 8-bit checksum.
    datalink_postamble_ic_t postamble;     ///< IC postamble bytes.
} PACK8;

/**
 * @brief Data link tail union for protocol abstraction.
 *
 * This union provides access to the tail fields for both IC and A5 protocol variants.
 * It enables unified handling of protocol-specific tail data, such as checksum and postamble,
 * for packet validation and parsing.
 */
union datalink_tail_t {
    datalink_tail_ic_t ic;  ///< Tail structure for IC protocol packets (includes checksum and postamble)
    datalink_tail_a5_t a5;  ///< Tail structure for A5 protocol packets (includes checksum)
};

uint8_t const DATALINK_MAX_TAIL_SIZE = sizeof(datalink_tail_t);

    // protocol preamble and postamble constants
extern datalink_preamble_a5_t  datalink_preamble_a5;   ///< A5 protocol preamble: { 0x00, 0xFF, 0xA5 }
extern datalink_preamble_ic_t  datalink_preamble_ic;   ///< IC protocol preamble: { 0x10, 0x02 }
extern datalink_postamble_ic_t datalink_postamble_ic;  ///< IC protocol postamble: { 0x10, 0x03 }

#if 0
/**
 * @brief Composes a device address from an address group and device ID.
 *
 * @param[in] group     The address group (high nibble).
 * @param[in] device_id The device ID within the group (low nibble).
 * @return              The composed 8-bit device address.
 */
datalink_addr_t datalink_addr(datalink_group_addr_t const group, datalink_pump_id_t const device_id);
#endif

/**
 * @brief Calculates the checksum for a data buffer.
 *
 * @param[in] start Pointer to the start of the data buffer.
 * @param[in] stop  Pointer to one past the end of the data buffer (exclusive).
 * @return          The calculated 16-bit checksum (sum of all bytes).
 */
uint16_t datalink_calc_checksum(uint8_t const * const start, uint8_t const * const stop);

/**
 * @brief Receive a protocol packet from the RS-485 bus.
 *
 * @param[in]  rs485 RS485 handle for reading bytes from the bus.
 * @param[out] pkt   Pointer to a packet structure to fill with received data.
 * @return           ESP_OK if a valid packet is received and checksum matches, ESP_FAIL otherwise.
 */
[[nodiscard]] esp_err_t datalink_rx_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt);

/**
 * @brief Adds protocol headers and tails to a data packet and queues it for RS485 transmission.
 *
 * @param[in] rs485 Pointer to the RS485 interface handle.
 * @param[in] pkt   Pointer to the datalink packet structure to be transmitted.
 */
void datalink_tx_pkt_queue(rs485_handle_t const rs485, datalink_pkt_t const * const pkt);

} // namespace opnpool
} // namespace esphome
