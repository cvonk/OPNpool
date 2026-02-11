/**
 * @file network_msg.h
 * @brief Defines the network message structures for the OPNpool component.
 *
 * @details
 * This header defines protocol-level message types, packed C-style structs for all
 * supported protocol messages, and lookup tables for message type metadata and size
 * validation.
 *
 * Message types are defined using the X-Macro pattern (NETWORK_MSG_TYP_LIST) which
 * generates the enum, size table, and protocol info table from a single definition,
 * ensuring they stay synchronized. To add a new message type, add a single line to
 * NETWORK_MSG_TYP_LIST.
 *
 * Thread safety is not provided, because it is not required for the single-threaded
 * nature of ESPHome.  UNKNOWN/unknown values have not yet been identified.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2014, 2019, 2022, 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#ifndef __cplusplus
# error "This header requires C++ compilation"
#endif

#include <esp_system.h>
#include <esp_types.h>
#include <cstddef>
#include <cstdint>
#include <algorithm>

#if defined(MAGIC_ENUM_RANGE_MIN)
# undef MAGIC_ENUM_RANGE_MIN
#endif
#if defined(MAGIC_ENUM_RANGE_MAX)
# undef MAGIC_ENUM_RANGE_MAX
#endif
#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include "utils/magic_enum.h"

#include "datalink_pkt.h"
#include "utils/to_str.h"
#include "utils/enum_helpers.h"

namespace esphome {
namespace opnpool {

    // operation modes of the pool controller.
    //  some say: bit0=service, bit2=celsius, bit3=freeze, bit7=timeout
struct network_ctrl_modes_t {
    uint8_t bits;
    bool is_service_mode()           const { return (bits & 0x01) != 0; }
    bool is_temp_increase_mode()     const { return (bits & 0x04) != 0; }
    bool is_freeze_protection_mode() const { return (bits & 0x08) != 0; }
    bool is_timeout_mode()           const { return (bits & 0x10) != 0; }   
} PACK8;

    // enumerates the pool controller circuits and features
enum class network_pool_circuit_t : uint8_t {
    SPA      = 0,  ///< Spa circuit (body 2)
    AUX1     = 1,  ///< Auxiliary circuit 1 (e.g. cleaner)
    AUX2     = 2,  ///< Auxiliary circuit 2 (e.g. air blower)
    AUX3     = 3,  ///< Auxiliary circuit 3 (e.g. light)
    FEATURE1 = 4,  ///< Feature 1
    POOL     = 5,  ///< Pool circuit (body 1)
    FEATURE2 = 6,  ///< Feature 2
    FEATURE3 = 7,  ///< Feature 3
    FEATURE4 = 8   ///< Feature 4
};

struct network_pump_run_mode_t {
    uint8_t raw;

    constexpr char const * to_str() const {
        switch (raw) {
            case  0: return "Filter";
            case  1: return "Manual";
            case  2: return "Backwash";
            case  3: return "Rinse";
            case  9: return "Extra Prg 1";
            case 10: return "Extra Prg 2";
            case 11: return "Extra Prg 3";
            case 12: return "Extra Prg 4";
            default: break;
        }
        return "Unknown";
    }
} PACK8;

struct network_pump_ctrl_t {
    uint8_t raw;
    constexpr bool is_local()  const { return raw != 0xFF; }
    constexpr bool is_remote() const { return raw == 0xFF; }
} PACK8;

    // enumerates the states of the pool pump
enum class network_pump_state_t : uint8_t {
    OK          = 0,  ///< Normal operation
    PRIMING     = 1,  ///< Priming state
    RUNNING     = 2,  ///< Running state
    UNKNOWN_03  = 3,
    SYS_PRIMING = 4   ///< System priming state
};

    // enumerates the heat sources for the pool heating system
    // names in camel case because this is as they appear in the UI
enum class network_heat_src_t : uint8_t {
    NONE,
    Heat,           ///< Heater
    SolarPreferred, ///< Solar preferred
    Solar           ///< Solar
};

struct network_lo_hi_t {
    uint8_t low;   ///< low order byte
    uint8_t high;  ///< high order byte
    constexpr uint16_t to_uint16() const { return ((uint16_t)high << 8) | low; }
} PACK8;

struct network_hi_lo_t {
    uint8_t high;  ///< high order byte
    uint8_t low;   ///< low order byte
    constexpr uint16_t to_uint16() const { return ((uint16_t)high << 8) | low; }
} PACK8;

/**
 * @brief Structures and unions for A5-controller messages.
 *
 * @details
 * This section provides packed C-style structs for each A5-controller style protocol message
 * exchanged between the pool controller and OPNpool.
 * 
 * @note The purpose of UNKNOWN* values has not yet been identified.
 * @note Details at https://github.com/tagyoureit/nodejs-poolController/blob/master/controller/comms/messages/status/EquipmentStateMessage.ts#L168
 */

struct network_ctrl_set_ack_t {
    datalink_ctrl_typ_t  typ;  // type that it is ACK'ing
} PACK8;

struct network_ctrl_circuit_set_t {
    uint8_t  circuit_plus_1;
    uint8_t  value;

    constexpr void set_value(bool v) { value = v ? 1 : 0; }
    constexpr bool get_value() const { return value != 0; }
} PACK8;

struct network_ctrl_sched_resp_sub_t {
    uint8_t         circuit_plus_1;  // 0  (0 = schedule not active)
    uint8_t         unknown_1;       // 1
    network_hi_lo_t prg_start;       // 2..3 [minutes]
    network_hi_lo_t prg_stop;        // 4..5 [minutes]
} PACK8;

constexpr size_t NETWORK_CTRL_SCHED_COUNT = 2;

struct network_ctrl_sched_resp_t {
    uint8_t                       unknown_0to3[4];  // 0,1,2,3
    network_ctrl_sched_resp_sub_t scheds[NETWORK_CTRL_SCHED_COUNT]; // 4,5,6,7,8,9, 10,11,12,13,14,15
} PACK8;

    // portable bit map for combined heat status
struct uint8_heat_status_t {
    uint8_t bits;
    bool get_pool() const { return bits & 0x04; }
    bool get_spa()  const { return bits & 0x08; }
    void set_pool(bool v) { bits = (bits & ~0x04) | (v ? 0x04 : 0); }
    void set_spa( bool v) { bits = (bits & ~0x08) | (v ? 0x08 : 0); }
} PACK8;

    // portable bit map for combined heat source
struct uint8_heat_src_t {
    uint8_t bits;  // lower nibble is pool heat source; higher nibble is for spa

    network_heat_src_t get_pool() const { return static_cast<network_heat_src_t>(bits & 0x0F); } 
    network_heat_src_t get_spa()  const { return static_cast<network_heat_src_t>((bits >> 4) & 0x0F); }    

    void set_pool(network_heat_src_t src) { bits = (bits & 0xF0) | (static_cast<uint8_t>(src) & 0x0F); }
    void set_spa( network_heat_src_t src) { bits = (bits & 0x0F) | (static_cast<uint8_t>(src) << 4); }
} PACK8;

struct network_time_t {
    uint8_t hour;   // 0
    uint8_t minute; // 1
} PACK8;

struct network_date_t {
    uint8_t day;   // 0
    uint8_t month; // 1
    uint8_t year;  // 2 (offset from 2000, e.g. 23 for 2023)
} PACK8;

struct network_ctrl_state_bcast_t {
    network_time_t       time;          // 0..1
    network_lo_hi_t      active;        // 2..3 bitmask for active circuits
    uint8_t              active_3;      // 4    bitmask for more active circuits
    uint8_t              active_4;      // 5    bitmask for more active circuits
    uint8_t              active_5;      // 6    bitmask for more active circuits
    uint8_t              unknown_07;    // 7
    uint8_t              unknown_08;    // 8
    network_ctrl_modes_t modes;         // 9    bitmask for active pool modes
    uint8_heat_status_t  heat_status;   // 10   bit2 is for POOL, bit3 is for SPA
    uint8_t              unknown_11;    // 11
    uint8_t              delay;         // 12   bitmask for delay status of circuits
    uint8_t              unknown_13;    // 13
    uint8_t              pool_temp;     // 14   water sensor 1 (POOL)
    uint8_t              spa_temp;      // 15   water sensor 2 (SPA, for shared system: mirrors water sensor 1)
    uint8_t              unknown_16;    // 16
    uint8_t              solar_temp_1;  // 17   solar sensor 1
    uint8_t              air_temp;      // 18   air sensor
    uint8_t              solar_temp_2;  // 19   solar sensor 2 (for shared system: mirrors solar sensor 1)
    uint8_t              unknown_20;    // 20   maybe water sensor 3
    uint8_t              unknown_21;    // 21   maybe water sensor 4
    uint8_heat_src_t     heat_src;      // 22   lowest nibble is for body 1 (POOL),  highest nibble is for body 2 (SPA)
    uint8_t              heat_src_2;    // 23   lowest nibble is for body 3, highest nibble is for body 4
    uint8_t              unknown_24;    // 24
    uint8_t              unknown_25;    // 25
    uint8_t              unknown_26;    // 26
    network_hi_lo_t      ocp_id;        // 27..28 outdoor control panel ID
} PACK8;

struct network_ctrl_time_t {
    network_time_t time;          // 0..1
    uint8_t        dayoftheweek;  // 2
    network_date_t date;          // 3..5
    uint8_t        clk_speed;     // 6
    uint8_t        dst_auto;      // 7 daylight savings time (1=auto, 0=manual)
} PACK8;

struct network_ctrl_version_resp_t {
    uint8_t req_id;              // 0
    uint8_t major;               // 1    0x02  -> version 2.___
    uint8_t minor;               // 2    0x50  -> version _.080
    uint8_t unknown_03to04[2];   // 3..4
    uint8_t boot_major;          // 5
    uint8_t boot_minor;          // 6   
    uint8_t unknown_07to16[10];  // 7..16
} PACK8;

struct network_ctrl_valve_resp_t {
    uint8_t unknown[24]; // 03 00 00 00 00 FF FF 01 02 03 04 01 48 00 00 00 03 00 00 00 04 00 00 00
} PACK8;

struct network_ctrl_solarpump_resp_t {
    uint8_t unknown[3];  // 05 00 00
} PACK8;

struct network_ctrl_delay_resp_t {
    uint8_t unknown[2];  // 10 00
} PACK8;

struct network_ctrl_heat_setpt_resp_t {
    uint8_t unknown[10];  // 00 00 00 00 00 00 00 00 00 00 
} PACK8;

struct network_ctrl_circ_names_req_t {
    uint8_t req_id;  // 0x01
} PACK8;

struct network_ctrl_circ_names_resp_t {
    uint8_t req_id;      // req 0x01 -> resp 01 01 48 00 00
    uint8_t unknown[5];  // req 0x02 -> resp 02 00 03 00 00
} PACK8;

struct network_ctrl_chem_req_t {
    uint8_t unknown;  // 0xD2
} PACK8;

struct network_ctrl_scheds_req_t {
    uint8_t sched_id;  // 0x01 (1 - 12)
} PACK8;

struct network_ctrl_scheds_resp_t {
    uint8_t                sched_id;     // 0 
    network_pool_circuit_t circuit;      // 1
    network_time_t         start;        // 2..3
    network_time_t         stop;         // 4..5
    uint8_t                day_of_week;  // 6 ///< bitmask Mon (0x01), Tue (0x02), Wed (0x04), Thu(0x08), Fri (0x10), Sat (0x20), Sun(0x40)
} PACK8;

struct network_ctrl_heat_resp_t {
    uint8_t          pool_temp;        // 0   ///< water sensor 1 (POOL)
    uint8_t          spa_temp;         // 1   ///< water sensor 2 (SPA)
    uint8_t          air_temp;         // 2   ///< air sensor
    uint8_t          pool_set_point;   // 3   ///< body 1 set-point (POOL)
    uint8_t          spa_set_point;    // 4   ///< body 2 set-point (SPA)
    uint8_heat_src_t heat_src;         // 5   ///< body 1 and 2 heat source, bits 0-3 for POOL, bits 4-7 for SPA
    uint8_t          water_temp_3;     // 6   ///< maybe water sensor 3
    uint8_t          water_temp_4;     // 7   ///< maybe water sensor 4
    uint8_t          air_temp_2;       // 8   ///< maybe reserved air sensor
    uint8_t          body_3_set_point; // 9   ///< maybe body 3 set-point
    uint8_t          body_4_set_point; // 10  ///< maybe body 4 set-point
    uint8_t          heat_src_3_4;     // 11  ///< maybe body 3 and 4 heat source
    uint8_t          unknown;          // 12
} PACK8;

struct network_ctrl_heat_set_t {
    uint8_t          pool_set_point;  // 0  ///< body 1 set-point (POOL)
    uint8_t          spa_set_point;   // 1  ///< body 2 set-point (SPA)
    uint8_heat_src_t heat_src;        // 2  ///< body 1 and 2 heat source, bits 0-3 for POOL, bits 4-7 for SPA
    uint8_t          unknown;         // 3
} PACK8;

struct network_ctrl_layout_t {
    network_ctrl_scheds_resp_t circuit[4];  // circuits assigned to each of the 4 buttons on the remote
} PACK8;

    // an informed guess towards the IntelliChlor message structure
struct network_intellichem_t {
    network_hi_lo_t ph;               // 0..1    ///< pH reading
    network_hi_lo_t orp;              // 2..3    ///< ORP reading
    network_hi_lo_t ph_setpoint;      // 4..5    ///< pH setpoint
    network_hi_lo_t orp_setpoint;     // 6..7    ///< ORP setpoint
    network_hi_lo_t tank;             // 8..9
    network_hi_lo_t calc_hardness;    // 10..11  ///< calcium hardness reading
    uint8_t         cya;              // 12      ///< cyanuric acid reading
    network_hi_lo_t total_alkalinity; // 13..14  ///< total alkalinity reading
    uint8_t         water_flow;       // 15      ///< water flow rate
    network_hi_lo_t mode;             // 16..17
} PACK8;

/**
 * @brief Defines the structures and unions for A5-pump messages.
 *
 * @details
 * This section provides packed C-style structs for each A5-pump style
 * protocol message exchanged between the pool controller and its pump.
 * 
 * @note  Details at https://github.com/tagyoureit/nodejs-poolController/blob/master/controller/comms/messages/status/PumpStateMessage.ts#L27
 */

enum class network_pump_reg_addr_t : uint8_t {
    RPM        = 0x01,  // program RPM
    POWER      = 0x02,  // program Power [Watt]
    CURRENT    = 0x03,  // program Current [A]
    STATUS     = 0x04,  // 0=off, 4=on, 10=running
    SETPOINT   = 0x05,
    TIMER_PROG = 0x06,
};

struct network_pump_reg_operation_t {
    uint8_t raw;

    static constexpr uint8_t WRITE = 0xC4;
    constexpr bool is_write() const { return raw == WRITE; }
    constexpr char const * to_str() const { return raw == WRITE ? "WRITE" : "READ"; }
} PACK8;


struct network_pump_reg_set_t {
    network_pump_reg_addr_t      address;    // 0
    network_pump_reg_operation_t operation;  // 1
    network_hi_lo_t              value;      // 2..3  0x0000 for read operation
} PACK8;

struct network_pump_reg_resp_t {
    network_hi_lo_t value;   // 0..1
} PACK8;

struct network_pump_running_t {
    uint8_t raw;
    
    constexpr bool is_on()  const { return (raw) == 0x0A; };
    constexpr bool is_off() const { return (raw) == 0x04; };
} PACK8;

#if 0
enum class network_pump_program_addr_t : uint16_t {
    UNKNOWN_2BF0 = 0x2BF0,
    UNKNOWN_02BF = 0x02BF,   
    PGM          = 0x02E4,  // program GPM
    RPM          = 0x02C4,  // program RPM
    EPRG         = 0x0321,  // select ext prog, 0x0000=P0, 0x0008=P1, 0x0010=P2, 0x0080=P3, 0x0020=P4
    ERPM0        = 0x0327,  // program ext program RPM0 (not turning, no speed)
    ERPM1        = 0x0328,  // program ext program RPM1
    ERPM2        = 0x0329,  // program ext program RPM2
    ERPM3        = 0x032A   // program ext program RPM3
};

/**
 * @brief          Converts a 16-bit pump program address to a string representation.
 *
 * @note           Can't use magic_enum because the enum values are not contiguous and outside magic_enum's range.
 *
 * @param[in] addr The pump program address to convert.
 * @return         String representation of the address.
 */
constexpr char const *
network_pump_program_addr_str(network_pump_program_addr_t const addr)
{
    switch (addr) {
        case network_pump_program_addr_t::UNKNOWN_2BF0: return "2BF0";
        case network_pump_program_addr_t::UNKNOWN_02BF: return "02BF";
        case network_pump_program_addr_t::PGM:          return "pgm";
        case network_pump_program_addr_t::RPM:          return "rpm";
        case network_pump_program_addr_t::EPRG:         return "eprg";
        case network_pump_program_addr_t::ERPM0:        return "erpm0";
        case network_pump_program_addr_t::ERPM1:        return "erpm1";
        case network_pump_program_addr_t::ERPM2:        return "erpm2";
        case network_pump_program_addr_t::ERPM3:        return "erpm3";
        default: return uint16_str(static_cast<uint16_t>(addr));
    }
}
#endif

struct network_pump_status_resp_t {
    network_pump_running_t   running;    // 0
    network_pump_run_mode_t  mode;       // 1
    network_pump_state_t     state;      // 2
    network_hi_lo_t          power;      // 3..4 [Watt]
    network_hi_lo_t          speed;      // 5..6 [rpm]
    uint8_t                  flow;       // 7 [G/min]
    uint8_t                  level;      // 8 [%]
    uint8_t                  unknown;    // 9
    uint8_t                  error;      // 10
    network_time_t           remaining;  // 11..12 (some say its status bits)
    network_time_t           clock;      // 13..14
} PACK8;
    

/**
 * @brief Defines the structures and unions for IC messages.
 *
 * @details
 * This section provides packed C-style structs for each IC style protocol message
 * exchanged  between the pool controller and the IntelliChlor chlorinator.
 * 
 * @note  Details at https://github.com/tagyoureit/nodejs-poolController/blob/master/controller/comms/messages/status/ChlorinatorStateMessage.ts
 */

struct network_chlor_control_req_t {
    uint8_t sub_cmd;

    static constexpr uint8_t CONTROL = 0x00;  // only known sub-command, used to request the chlorinator control/status
    static constexpr network_chlor_control_req_t control() { return network_chlor_control_req_t{CONTROL}; }
    constexpr bool is_control_req() const { return sub_cmd == CONTROL; }
} PACK8;

using network_chlor_name_t = char[16];

struct network_chlor_control_resp_t {
    uint8_t  unknown[2];  // always appear to be 00 00
} PACK8;

struct network_chlor_model_req_t {
    uint8_t typ;

    static constexpr uint8_t TYP = 0x00;  // only known type, used to request the chlorinator model
    static constexpr network_chlor_model_req_t set_typ() { return network_chlor_model_req_t{TYP}; }
    constexpr bool is_get_typ() const { return typ; }
} PACK8;

struct network_chlor_model_resp_t {
    uint8_t              salt;  ///< parts per million /50
    network_chlor_name_t name;  ///< non-\0 terminated chlorinator name
} PACK8;

struct network_chlor_level_set_t {
    uint8_t  level;             ///< chlorine level percentage (0-100)
} PACK8;

struct network_chlor_level10_set_t {
    uint8_t  level_times_10;    ///< chlorine level times 10 (0-255, representing 0.0% to 25.5%)
} PACK8;

struct network_chlor_level_resp_t {
    uint8_t  salt;              ///< Parts per million /50
    uint8_t  error;             ///< error bits: low flow (0x01), low salt (0x02), high salt (0x04), clean cell (0x10), cold (0x40), OK (0x80)
} PACK8;

struct network_chlor_ichlor_bcast_t {
    uint8_t  level;             ///< current chlorine level percentage
    uint8_t  temp;              ///< water temperature
} PACK8;


/**
 * @brief Defines unions for grouping protocol message data.
 *
 * @details
 * These unions encapsulate all supported message types for the A5 (controller/pump) and IC
 * (chlorinator) protocols, allowing flexible access to protocol-specific message structures.
 * The top-level union `network_data_t` enables generic handling of any protocol message
 * within the OPNpool system, simplifying encoding, decoding, and processing of network
 * messages.
 * 
 * @note These represent the non-0 length messages
 */
union network_data_a5_t {
    network_pump_reg_set_t         pump_reg_set;
    network_pump_reg_resp_t        pump_reg_resp;
    network_pump_ctrl_t            pump_ctrl;         // set or resp
    network_pump_run_mode_t        pump_mode;         // set or resp
    network_pump_running_t         pump_running;      // set or resp
    network_pump_status_resp_t     pump_status_resp;
    network_ctrl_set_ack_t         ctrl_set_ack;
    network_ctrl_circuit_set_t     ctrl_circuit_set;
    network_ctrl_sched_resp_t      ctrl_sched_resp;
    network_ctrl_state_bcast_t     ctrl_state_bcast;
    network_ctrl_time_t            ctrl_time;         // set or resp
    network_ctrl_heat_resp_t       ctrl_heat_resp;
    network_ctrl_heat_set_t        ctrl_heat_set;
    network_ctrl_layout_t          ctrl_layout_resp;
    network_ctrl_layout_t          ctrl_layout_set;    
    network_ctrl_valve_resp_t      ctrl_valve_resp;
    network_ctrl_version_resp_t    ctrl_version_resp;
    network_ctrl_solarpump_resp_t  ctrl_solarpump_resp;
    network_ctrl_delay_resp_t      ctrl_delay_resp;
    network_ctrl_heat_setpt_resp_t ctrl_heat_setpt_resp;
    network_ctrl_circ_names_req_t  ctrl_circ_names_req;
    network_ctrl_circ_names_resp_t ctrl_circ_names_resp;
    network_ctrl_scheds_req_t      ctrl_scheds_req;
    network_ctrl_scheds_resp_t     ctrl_scheds_resp;
    network_ctrl_chem_req_t        ctrl_chem_req;
} PACK8;

union network_data_ic_t {
    network_chlor_control_req_t  chlor_control_req;
    network_chlor_control_resp_t chlor_status_resp;
    network_chlor_model_req_t    chlor_model_req;
    network_chlor_model_resp_t   chlor_model_resp;
    network_chlor_level_set_t    chlor_level_set;
    network_chlor_level10_set_t  chlor_level10_set;
    network_chlor_level_resp_t   chlor_level_resp;
} PACK8;

inline constexpr uint8_t DATALINK_MAX_DATA_SIZE = std::max(sizeof(network_data_a5_t), sizeof(network_data_ic_t));

union network_data_t {
    network_data_a5_t a5;
    network_data_ic_t ic;
    uint8_t           raw[DATALINK_MAX_DATA_SIZE];
} PACK8;


/**
 * @brief X-Macro defining all supported network message types for OPNpool.
 *
 * @details
 * This macro generates the enum values, size lookup table, and protocol info table
 * from a single definition, ensuring they stay synchronized.
 *
 * Format: X(ENUM_NAME, SIZE_EXPR, IS_TO_PUMP, PROTOCOL, DATALINK_TYPE)
 *   - ENUM_NAME: The enum value name
 *   - SIZE_EXPR: sizeof(struct) for messages with data, 0 for empty messages
 *   - IS_TO_PUMP: Whether the message is sent to the pump (true) or from the pump (false)
 *   - PROTOCOL: datalink_prot_t value (A5_PUMP, A5_CTRL, or IC)
 *   - DATALINK_TYPE: The datalink type enum value
 * 
 * @note In C++ empty structs have a size of 1, not 0.  For those we use 0 in this table.
 */
#define NETWORK_MSG_TYP_LIST(X) \
    X(IGNORE,                1,                                     false, A5_PUMP, datalink_pump_typ_t::REJECTING)    \
    X(PUMP_REG_SET,          sizeof(network_pump_reg_set_t),        true,  A5_PUMP, datalink_pump_typ_t::REG)          \
    X(PUMP_REG_RESP,         sizeof(network_pump_reg_resp_t),       false, A5_PUMP, datalink_pump_typ_t::REG)          \
    X(PUMP_REG_VF_SET,       sizeof(network_pump_reg_set_t),        true,  A5_PUMP, datalink_pump_typ_t::REG_VF)       \
    X(PUMP_REG_VF_RESP,      sizeof(network_pump_reg_resp_t),       false, A5_PUMP, datalink_pump_typ_t::REG_VF)       \
    X(PUMP_REG_VS_SET,       sizeof(network_pump_reg_set_t),        true,  A5_PUMP, datalink_pump_typ_t::REG_VS)       \
    X(PUMP_REG_VS_RESP,      sizeof(network_pump_reg_resp_t),       false, A5_PUMP, datalink_pump_typ_t::REG_VS)       \
    X(PUMP_REMOTE_CTRL_SET,  sizeof(network_pump_ctrl_t),           true,  A5_PUMP, datalink_pump_typ_t::REMOTE_CTRL)  \
    X(PUMP_REMOTE_CTRL_RESP, sizeof(network_pump_ctrl_t),           false, A5_PUMP, datalink_pump_typ_t::REMOTE_CTRL)  \
    X(PUMP_RUN_MODE_SET,     sizeof(network_pump_run_mode_t),       true,  A5_PUMP, datalink_pump_typ_t::RUN_MODE)     \
    X(PUMP_RUN_MODE_RESP,    sizeof(network_pump_run_mode_t),       false, A5_PUMP, datalink_pump_typ_t::RUN_MODE)     \
    X(PUMP_RUN_SET,          sizeof(network_pump_running_t),        true,  A5_PUMP, datalink_pump_typ_t::RUN)          \
    X(PUMP_RUN_RESP,         sizeof(network_pump_running_t),        false, A5_PUMP, datalink_pump_typ_t::RUN)          \
    X(PUMP_STATUS_REQ,       0,                                     true,  A5_PUMP, datalink_pump_typ_t::STATUS)       \
    X(PUMP_STATUS_RESP,      sizeof(network_pump_status_resp_t),    false, A5_PUMP, datalink_pump_typ_t::STATUS)       \
    X(CTRL_SET_ACK,          sizeof(network_ctrl_set_ack_t),        false, A5_CTRL, datalink_ctrl_typ_t::SET_ACK)      \
    X(CTRL_CIRCUIT_SET,      sizeof(network_ctrl_circuit_set_t),    false, A5_CTRL, datalink_ctrl_typ_t::CIRCUIT_SET)  \
    X(CTRL_SCHED_REQ,        0,                                     false, A5_CTRL, datalink_ctrl_typ_t::SCHED_REQ)    \
    X(CTRL_SCHED_RESP,       sizeof(network_ctrl_sched_resp_t),     false, A5_CTRL, datalink_ctrl_typ_t::SCHED_RESP)   \
    X(CTRL_STATE_BCAST,      sizeof(network_ctrl_state_bcast_t),    false, A5_CTRL, datalink_ctrl_typ_t::STATE_BCAST)  \
    X(CTRL_TIME_REQ,         0,                                     false, A5_CTRL, datalink_ctrl_typ_t::TIME_REQ)     \
    X(CTRL_TIME_RESP,        sizeof(network_ctrl_time_t),           false, A5_CTRL, datalink_ctrl_typ_t::TIME_RESP)    \
    X(CTRL_TIME_SET,         sizeof(network_ctrl_time_t),           false, A5_CTRL, datalink_ctrl_typ_t::TIME_SET)     \
    X(CTRL_HEAT_REQ,         0,                                     false, A5_CTRL, datalink_ctrl_typ_t::HEAT_REQ)     \
    X(CTRL_HEAT_RESP,        sizeof(network_ctrl_heat_resp_t),      false, A5_CTRL, datalink_ctrl_typ_t::HEAT_RESP)    \
    X(CTRL_HEAT_SET,         sizeof(network_ctrl_heat_set_t),       false, A5_CTRL, datalink_ctrl_typ_t::HEAT_SET)     \
    X(CTRL_LAYOUT_REQ,       0,                                     false, A5_CTRL, datalink_ctrl_typ_t::LAYOUT_REQ)   \
    X(CTRL_LAYOUT_RESP,      sizeof(network_ctrl_layout_t),         false, A5_CTRL, datalink_ctrl_typ_t::LAYOUT_RESP)  \
    X(CTRL_LAYOUT_SET,       sizeof(network_ctrl_layout_t),         false, A5_CTRL, datalink_ctrl_typ_t::LAYOUT_SET)   \
    X(CTRL_VALVE_REQ,        0,                                     false, A5_CTRL, datalink_ctrl_typ_t::VALVE_REQ)    \
    X(CTRL_VALVE_RESP,       sizeof(network_ctrl_valve_resp_t),     false, A5_CTRL, datalink_ctrl_typ_t::VALVE_RESP)   \
    X(CTRL_VERSION_REQ,      0,                                     false, A5_CTRL, datalink_ctrl_typ_t::VERSION_REQ)  \
    X(CTRL_VERSION_RESP,     sizeof(network_ctrl_version_resp_t),   false, A5_CTRL, datalink_ctrl_typ_t::VERSION_RESP) \
    X(CTRL_SOLARPUMP_REQ,    0,                                     false, A5_CTRL, datalink_ctrl_typ_t::SOLARPUMP_REQ)   \
    X(CTRL_SOLARPUMP_RESP,   sizeof(network_ctrl_solarpump_resp_t), false, A5_CTRL, datalink_ctrl_typ_t::SOLARPUMP_RESP)  \
    X(CTRL_DELAY_REQ,        0,                                     false, A5_CTRL, datalink_ctrl_typ_t::DELAY_REQ)       \
    X(CTRL_DELAY_RESP,       sizeof(network_ctrl_delay_resp_t),     false, A5_CTRL, datalink_ctrl_typ_t::DELAY_RESP)      \
    X(CTRL_HEAT_SETPT_REQ,   0,                                     false, A5_CTRL, datalink_ctrl_typ_t::HEAT_SETPT_REQ)  \
    X(CTRL_HEAT_SETPT_RESP,  sizeof(network_ctrl_heat_setpt_resp_t),false, A5_CTRL, datalink_ctrl_typ_t::HEAT_SETPT_RESP) \
    X(CTRL_CIRC_NAMES_REQ,   sizeof(network_ctrl_circ_names_req_t), false, A5_CTRL, datalink_ctrl_typ_t::CIRC_NAMES_REQ)  \
    X(CTRL_CIRC_NAMES_RESP,  sizeof(network_ctrl_circ_names_resp_t),false, A5_CTRL, datalink_ctrl_typ_t::CIRC_NAMES_RESP) \
    X(CTRL_SCHEDS_REQ,       sizeof(network_ctrl_scheds_req_t),     false, A5_CTRL, datalink_ctrl_typ_t::SCHEDS_REQ)    \
    X(CTRL_SCHEDS_RESP,      sizeof(network_ctrl_scheds_resp_t),    false, A5_CTRL, datalink_ctrl_typ_t::SCHEDS_RESP)   \
    X(CTRL_CHEM_REQ,         sizeof(network_ctrl_chem_req_t),       false, A5_CTRL, datalink_ctrl_typ_t::CHEM_REQ)      \
    X(CHLOR_CONTROL_REQ,     sizeof(network_chlor_control_req_t),   false, IC,      datalink_chlor_typ_t::CONTROL_REQ)  \
    X(CHLOR_CONTROL_RESP,    sizeof(network_chlor_control_resp_t),  false, IC,      datalink_chlor_typ_t::CONTROL_RESP) \
    X(CHLOR_MODEL_REQ,       sizeof(network_chlor_model_req_t),     false, IC,      datalink_chlor_typ_t::MODEL_REQ)    \
    X(CHLOR_MODEL_RESP,      sizeof(network_chlor_model_resp_t),    false, IC,      datalink_chlor_typ_t::MODEL_RESP)   \
    X(CHLOR_LEVEL_SET,       sizeof(network_chlor_level_set_t),     false, IC,      datalink_chlor_typ_t::LEVEL_SET)    \
    X(CHLOR_LEVEL_SET10,     sizeof(network_chlor_level10_set_t),   false, IC,      datalink_chlor_typ_t::LEVEL_SET10)  \
    X(CHLOR_LEVEL_RESP,      sizeof(network_chlor_level_resp_t),    false, IC,      datalink_chlor_typ_t::LEVEL_RESP)   \
    X(CHLOR_ICHLOR_BCAST,    sizeof(network_chlor_ichlor_bcast_t),  false, IC,      datalink_chlor_typ_t::ICHLOR_BCAST)

/**
 * @brief Enumerates all supported network message types for OPNpool.
 *
 * @details
 * Each value represents a specific protocol message exchanged between the ESPHome
 * component and pool equipment, including controller, pump, and chlorinator messages.
 * Used for message dispatch, parsing, and type-safe handling. Generated from
 * NETWORK_MSG_TYP_LIST X-Macro.
 */
enum class network_msg_typ_t : uint8_t {
#define X_ENUM(name, size, is_to_pump, proto, typ) name,
    NETWORK_MSG_TYP_LIST(X_ENUM)
#undef X_ENUM
};

/**
 * @brief Metadata structure for network message types.
 *
 * Contains protocol info, datalink type, message size, direction, and the corresponding
 * network_msg_typ_t value. Used for message type lookup and validation.
 */
struct network_msg_typ_info_t {
    datalink_prot_t    proto;
    datalink_typ_t     datalink_typ;
    uint32_t           size;
    bool               is_to_pump;
    network_msg_typ_t  network_msg_typ;
    
        // each constructor overload handles a different datalink type enum, allowing the X-Macro to pass the correct type
    constexpr network_msg_typ_info_t(datalink_prot_t dp, uint8_t pt, uint32_t s, bool itp, network_msg_typ_t nt)
        : proto(dp), datalink_typ{.raw = pt}, size(s), is_to_pump(itp), network_msg_typ(nt) {}

    constexpr network_msg_typ_info_t(datalink_prot_t dp, datalink_ctrl_typ_t dct, uint32_t s, bool itp, network_msg_typ_t nt)
        : proto(dp), datalink_typ{.ctrl = dct}, size(s), is_to_pump(itp), network_msg_typ(nt) {}

    constexpr network_msg_typ_info_t(datalink_prot_t dp, datalink_pump_typ_t dpt, uint32_t s, bool itp, network_msg_typ_t nt)
        : proto(dp), datalink_typ{.pump = dpt}, size(s), is_to_pump(itp), network_msg_typ(nt) {}

    constexpr network_msg_typ_info_t(datalink_prot_t dp, datalink_chlor_typ_t dct, uint32_t s, bool itp, network_msg_typ_t nt)
        : proto(dp), datalink_typ{.chlor = dct}, size(s), is_to_pump(itp), network_msg_typ(nt) {}
};

    // maps {datalink_prot and datalink_typ_t} to network_msg_typ_t.
constexpr network_msg_typ_info_t network_msg_typ_info[] = {
#define X_INFO(name, size, is_to_pump, proto, typ) {datalink_prot_t::proto, typ, size, is_to_pump, network_msg_typ_t::name},
    NETWORK_MSG_TYP_LIST(X_INFO)
#undef X_INFO
};

/**
 * @brief     Looks up message type info by network_msg_typ_t.
 *
 * @param[in] typ The network message type to look up.
 * @return        Pointer to the matching network_msg_typ_info_t, or nullptr if not found.
 */
constexpr network_msg_typ_info_t const *
network_msg_typ_get_info(network_msg_typ_t typ)
{
    uint8_t idx = static_cast<uint8_t>(typ);
    if (idx < std::size(network_msg_typ_info)) {
        return &network_msg_typ_info[idx];
    }
    return nullptr;
}

/**
 * @brief              Reverse lookup from (datalink_prot_t, datalink_ctrl_typ_t) to network_msg_typ_info_t.
 *
 * @param[in] ctrl_typ The datalink controller message type.
 * @return             Pointer to the matching network_msg_typ_info_t, or nullptr if not found.
 */
constexpr network_msg_typ_info_t const *
network_msg_typ_get_info(datalink_ctrl_typ_t const ctrl_typ)
{
    for (size_t ii = 0; ii < std::size(network_msg_typ_info); ii++) {
        network_msg_typ_info_t const * const info = &network_msg_typ_info[ii];
        if (info->proto == datalink_prot_t::A5_CTRL && info->datalink_typ.ctrl == ctrl_typ) {
            return info;
        }
    }
    return nullptr;
}

/**
 * @brief                Reverse lookup from (datalink_prot_t, datalink_pump_typ_t) to network_msg_typ_info_t.
 *
 * @param[in] pump_typ   The datalink pump message type.
 * @param[in] is_to_pump True if message is directed to pump (SET/REQ), false if from pump (RESP).
 * @return               Pointer to the matching network_msg_typ_info_t, or nullptr if not found.
 */
constexpr network_msg_typ_info_t const *
network_msg_typ_get_info(datalink_pump_typ_t const pump_typ, bool const is_to_pump)
{
    for (size_t ii = 0; ii < std::size(network_msg_typ_info); ii++) {
        network_msg_typ_info_t const * const info = &network_msg_typ_info[ii];
        if (info->proto == datalink_prot_t::A5_PUMP && info->datalink_typ.pump == pump_typ && info->is_to_pump == is_to_pump) {
            return info;
        }
    }
    return nullptr;
}

/**
 * @brief               Reverse lookup from (datalink_prot_t, datalink_chlor_typ_t) to network_msg_typ_info_t.
 *
 * @param[in] chlor_typ The datalink chlorinator message type.
 * @return              Pointer to the matching network_msg_typ_info_t, or nullptr if not found.
 */
constexpr network_msg_typ_info_t const *
network_msg_typ_get_info(datalink_chlor_typ_t const chlor_typ)
{
    for (size_t ii = 0; ii < std::size(network_msg_typ_info); ii++) {
        network_msg_typ_info_t const * const info = &network_msg_typ_info[ii];
        if (info->proto == datalink_prot_t::IC && info->datalink_typ.chlor == chlor_typ) {
            return info;
        }
    }
    return nullptr;
}

/**
 * @brief Represents a generic network message for the Pentair protocol.
 *
 * @details
 * Contains the message type and a union of all possible protocol-specific message data 
 * structures, allowing flexible handling of controller, pump, and chlorinator messages.
 */
struct network_msg_t {
    datalink_addr_t    src;  ///< source address from datalink_hdr
    datalink_addr_t    dst;  ///< destination address from datalink_hdr
    network_msg_typ_t  typ;  ///< The network message type identifier.
    network_data_t     u;    ///< Union containing all supported message data structures for A5/controller, A5/pump, and IC messages.
};

    // sanity checks
static_assert(sizeof(uint8_heat_status_t) == 1, "uint8_heat_status_t must be 1 byte");
static_assert(sizeof(uint8_heat_src_t)    == 1, "uint8_heat_src_t must be 1 byte");
static_assert(sizeof(network_data_t)      <= UINT8_MAX, "network_data_t size exceeds UINT8_MAX");
static_assert(std::size(network_msg_typ_info) == enum_count<network_msg_typ_t>());
static_assert(network_msg_typ_get_info(datalink_pump_typ_t::STATUS, true)  == &network_msg_typ_info[enum_index(network_msg_typ_t::PUMP_STATUS_REQ)]);
static_assert(network_msg_typ_get_info(datalink_pump_typ_t::STATUS, false) == &network_msg_typ_info[enum_index(network_msg_typ_t::PUMP_STATUS_RESP)]);
static_assert(network_msg_typ_get_info(datalink_ctrl_typ_t::STATE_BCAST)   == &network_msg_typ_info[enum_index(network_msg_typ_t::CTRL_STATE_BCAST)]);
static_assert(network_msg_typ_get_info(datalink_chlor_typ_t::LEVEL_RESP)   == &network_msg_typ_info[enum_index(network_msg_typ_t::CHLOR_LEVEL_RESP)]);

}  // namespace opnpool
}  // namespace esphome