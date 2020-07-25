#pragma once
#include <esp_system.h>

#include "../datalink/datalink.h"

#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif
#if !defined CONFIG_POOL_DBG_NETWORK
# define CONFIG_POOL_DBG_NETWORK (0)
#endif
#if !defined CONFIG_POOL_DBG_NETWORK_ONERROR
# define CONFIG_POOL_DBG_NETWORK_ONERROR (0)
#endif

/* results of sending requests:
DATALINK_CTRL_TYP_UNKNOWNxCB = 0xCB, // sending [],   returns: 01 01 48 00 00
DATALINK_CTRL_TYP_UNKNOWNxD1 = 0xD1, // sending [],   returns: 01 06 00 00 00 00 3F
DATALINK_CTRL_TYP_UNKNOWNxD9 = 0xD9, // sending [],   returns: 11 3C 00 3F 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
DATALINK_CTRL_TYP_UNKNOWNxDD = 0xDD, // sending [],   returns: 03 00 00 00 00 FF FF 01 02 03 04 01 48 00 00 00 03 00 00 00 04 00 00 00
DATALINK_CTRL_TYP_UNKNOWNxE2 = 0xE2, // sending [],   returns: 05 00 00 => looks like a boring ic ping request
DATALINK_CTRL_TYP_UNKNOWNxE3 = 0xE3, // sending [],   returns: 10 00
DATALINK_CTRL_TYP_UNKNOWNxE8 = 0xE8, // sending [],   returns: 00 00 00 00 00 00 00 00 00 00
DATALINK_CTRL_TYP_UNKNOWN_FD = 0xFD, // sending [],   returns: 01 02 50 00 00 00 00 00 00 00 00 00 00 00 00 00 00
*/

/**
 * macro "magic" to get an enum and matching *_str functions (in *_str.c)
 **/

#define NETWORK_ADDRGROUP_MAP(XX) \
  XX(0x00, ALL)  \
  XX(0x01, CTRL) \
  XX(0x02, REMOTE) \
  XX(0x05, CHLOR) \
  XX(0x06, PUMP) \
  XX(0x09, X09)

typedef enum {
#define XX(num, name) NETWORK_ADDRGROUP_##name = num,
  NETWORK_ADDRGROUP_MAP(XX)
#undef XX
} network_addrgroup_t;

#define NETWORK_CIRCUIT_MAP(XX) \
  XX( 1, SPA)  \
  XX( 2, AUX1) \
  XX( 3, AUX2) \
  XX( 4, AUX3) \
  XX( 5, FT1)  \
  XX( 6, POOL) \
  XX( 7, FT2)  \
  XX( 8, FT3)  \
  XX( 9, FT4)  \
  XX(10, COUNT)

typedef enum {
#define XX(num, name) NETWORK_CIRCUIT_##name = num,
  NETWORK_CIRCUIT_MAP(XX)
#undef XX
} network_circuit_t;

#define NETWORK_PUMP_MODE_MAP(XX) \
  XX(0, FILTER)  \
  XX(1, MAN)  \
  XX(2, BKWASH)  \
  XX(3, X03)  \
  XX(4, X04)  \
  XX(5, X05)  \
  XX(6, FT1)  \
  XX(7, X07)  \
  XX(8, X08)  \
  XX(9, EP1)  \
  XX(10, EP2)  \
  XX(11, EP3)  \
  XX(12, EP4)

typedef enum {
#define XX(num, name) NETWORK_PUMP_MODE_##name = num,
  NETWORK_PUMP_MODE_MAP(XX)
#undef XX
} network_pump_mode_t;

#define NETWORK_HEAT_SRC_MAP(XX) \
  XX(0, NONE)       \
  XX(1, HEATER)     \
  XX(2, SOLAR_PREF) \
  XX(3, SOLAR)

typedef enum {
#define XX(num, name) NETWORK_HEAT_SRC_##name = num,
  NETWORK_HEAT_SRC_MAP(XX)
#undef XX
} network_heat_src_t;

// use macro "magic" to get an enum and matching name_* function (in name.c)
#define NETWORK_MSG_TYP_MAP(XX) \
  XX( 0, NONE)              \
  XX( 1, PUMP_REG_SET)      \
  XX( 2, PUMP_REG_SET_RESP) \
  XX( 3, PUMP_CTRL)         \
  XX( 4, PUMP_MODE)         \
  XX( 5, PUMP_RUNNING)      \
  XX( 6, PUMP_STATE_REQ)   \
  XX( 7, PUMP_STATE)       \
  XX( 8, CTRL_SET_ACK)      \
  XX( 9, CTRL_CIRCUIT_SET)  \
  XX(10, CTRL_SCHED_REQ)    \
  XX(11, CTRL_SCHED)        \
  XX(12, CTRL_STATE_REQ)    \
  XX(13, CTRL_STATE)        \
  XX(14, CTRL_STATE_SET)    \
  XX(15, CTRL_TIME_REQ)     \
  XX(16, CTRL_TIME)         \
  XX(17, CTRL_TIME_SET)     \
  XX(18, CTRL_HEAT_REQ)     \
  XX(19, CTRL_HEAT)         \
  XX(20, CTRL_HEAT_SET)     \
  XX(21, CTRL_LAYOUT_REQ)   \
  XX(22, CTRL_LAYOUT)       \
  XX(23, CTRL_LAYOUT_SET)   \
  XX(24, CHLOR_PING_REQ)    \
  XX(25, CHLOR_PING)        \
  XX(26, CHLOR_NAME)        \
  XX(27, CHLOR_LEVEL_SET)   \
  XX(28, CHLOR_LEVEL_RESP)

typedef enum {
#define XX(num, name) NETWORK_MSG_TYP_##name = num,
  NETWORK_MSG_TYP_MAP(XX)
#undef XX
} network_msg_typ_t;

/*
 * A5 messages, used to communicate with components except IntelliChlor
 */

typedef struct network_msg_ctrl_set_ack_t {
    uint8_t typ;
} PACK8 network_msg_ctrl_set_ack_t;

typedef struct network_msg_ctrl_state_t {
    uint8_t hour;               // 0
    uint8_t minute;             // 1
    uint8_t activeLo;           // 2
    uint8_t activeHi;           // 3
    uint8_t UNKNOWN_4to8[5];    // 4..8
    uint8_t remote;             // 9
    uint8_t heating;            // 10
    uint8_t UNKNOWN_11;         // 11
    uint8_t delay;              // 12
    uint8_t UNKNOWN_13;         // 13
    uint8_t poolTemp;           // 14
    uint8_t spaTemp;            // 15
    uint8_t major;              // 16
    uint8_t minor;              // 17
    uint8_t airTemp;            // 18
    uint8_t solarTemp;          // 19
    uint8_t UNKNOWN_20;         // 20
    uint8_t UNKNOWN_21;         // 21
    uint8_t heatSrc;            // 22
    uint8_t UNKNOWN_23to28[6];  // 23..28
} PACK8 network_msg_ctrl_state_t;

typedef struct network_msg_ctrl_time_t {
    uint8_t hour;            // 0
    uint8_t minute;          // 1
    uint8_t UNKNOWN_2;        // 2 (DST adjust?)
    uint8_t day;             // 3
    uint8_t month;           // 4
    uint8_t year;            // 5
    uint8_t clkSpeed;        // 6
    uint8_t daylightSavings; // 7
} PACK8 network_msg_ctrl_time_t;

typedef network_msg_ctrl_time_t network_msg_ctrl_time_set_t;

typedef struct network_msg_ctrl_heat_t {
    uint8_t poolTemp;          // 0
    uint8_t spaTemp;           // 1
    uint8_t airTemp;           // 2
    uint8_t poolTempSetpoint;  // 3
    uint8_t spaTempSetpoint;   // 4
    uint8_t heatSrc;           // 5
    uint8_t UNKNOWN_6;         // 6
    uint8_t UNKNOWN_7;         // 7
    uint8_t UNKNOWN_8;         // 8
    uint8_t UNKNOWN_9;         // 9
    uint8_t UNKNOWN_10;        // 10
    uint8_t UNKNOWN_11;        // 11
    uint8_t UNKNOWN_12;        // 12
} PACK8 network_msg_ctrl_heat_t;

typedef struct network_msg_ctrl_heat_set_t {
    uint8_t poolTempSetpoint;  // 0
    uint8_t spaTempSetpoint;   // 1
    uint8_t heatSrc;           // 2
    uint8_t UNKNOWN_3;         // 3
} PACK8 network_msg_ctrl_heat_set_t;

typedef struct network_msg_ctrl_circuit_set_t {
    uint8_t circuit;
    uint8_t value;
} PACK8 network_msg_ctrl_circuit_set_t;

typedef struct mCtrlSchedSub_a5_t {
    uint8_t circuit;            // 0
    uint8_t UNKNOWN_1;          // 1
    uint8_t prgStartHi;         // 2 [min]
    uint8_t prgStartLo;         // 3 [min]
    uint8_t prgStopHi;          // 4 [min]
    uint8_t prgStopLo;          // 5 [min]
} PACK8 mCtrlSchedSub_a5_t;

typedef struct network_msg_ctrl_sched_t {
    uint8_t UNKNOWN_0to3[4];      // 0,1,2,3
    mCtrlSchedSub_a5_t scheds[2];  // 4,5,6,7,8,9, 10,11,12,13,14,15
} PACK8 network_msg_ctrl_sched_t;

typedef struct network_msg_ctrl_layout_t {
    uint8_t circuit[4];  // 0..3 corresponding to the buttons on the remote
} PACK8 network_msg_ctrl_layout_t;

typedef network_msg_ctrl_layout_t network_msg_ctrl_layout_set_t;

typedef struct network_msg_pump_reg_set_t {
    uint8_t addressHi;   // 0
    uint8_t addressLo;   // 1
    uint8_t valueHi;     // 2
    uint8_t valueLo;     // 3
} PACK8 network_msg_pump_reg_set_t;

typedef struct network_msg_pump_reg_resp_t {
    uint8_t valueHi;     // 0
    uint8_t valueLo;     // 1
} PACK8 network_msg_pump_reg_resp_t;

typedef struct network_msg_pump_ctrl_t {
    uint8_t ctrl;     // 0
} PACK8 network_msg_pump_ctrl_t;

typedef struct network_msg_pump_mode_t {
    uint8_t mode;        // 0
} PACK8 network_msg_pump_mode_t;

typedef struct network_msg_pump_running_t {
    uint8_t running;       // 0
} PACK8 network_msg_pump_running_t;

typedef struct network_msg_pump_state_t {
    uint8_t running;     // 0
    uint8_t mode;        // 1
    uint8_t status;      // 2
    uint8_t powerHi;     // 3
    uint8_t powerLo;     // 4 [Watt]
    uint8_t rpmHi;       // 5
    uint8_t rpmLo;       // 6 [rpm]
    uint8_t gpm;         // 7 [G/min]
    uint8_t pct;         // 8 [%]
    uint8_t UNKNOWN_9;   // 9
    uint8_t err;         // 10
    uint8_t UNKNOWN_11;  // 11
    uint8_t timer;       // 12 [min]
    uint8_t hour;        // 13
    uint8_t minute;      // 14
} PACK8 network_msg_pump_state_t;

/*
* IC messages, use to communicate with IntelliChlor
*/

typedef struct network_msg_chlor_ping_req_t {
    uint8_t UNKNOWN_0;
} PACK8 network_msg_chlor_ping_req_t;

typedef struct network_msg_chlor_ping_t {
    uint8_t UNKNOWN_0;
    uint8_t UNKNOWN_1;
} PACK8 network_msg_chlor_ping_t;

typedef char network_msg_chlor_name_str_t[16];

typedef struct network_msg_chlor_name_t {
    uint8_t                     UNKNOWN_0;
    network_msg_chlor_name_str_t name;
} PACK8 network_msg_chlor_name_t;

typedef struct network_msg_chlor_level_set_t {
    uint8_t pct;
} PACK8 network_msg_chlor_level_set_t;

typedef struct network_msg_chlor_level_resp_t {
    uint8_t salt;
    uint8_t err;  // call ; cold water
} PACK8 network_msg_chlor_level_resp_t;

typedef struct mChlor0X14_ic_t {
    uint8_t UNKNOWN_0;
} PACK8 mChlor0X14_ic_t;

typedef struct network_msg_t {
    network_msg_typ_t typ;
    union {
        network_msg_pump_reg_set_t * pump_reg_set;
        network_msg_pump_reg_resp_t * pump_reg_set_resp;
        network_msg_pump_ctrl_t * pump_ctrl;
        network_msg_pump_mode_t * pump_mode;
        network_msg_pump_running_t * pump_running;
        network_msg_pump_state_t * pump_state;
        network_msg_ctrl_set_ack_t * ctrl_set_ack;
        network_msg_ctrl_circuit_set_t * ctrl_circuit_set;
        network_msg_ctrl_sched_t * ctrl_sched;
        network_msg_ctrl_state_t * ctrl_state;
        network_msg_ctrl_time_t * ctrl_time;
        network_msg_ctrl_time_set_t * ctrl_time_set;
        network_msg_ctrl_heat_t * ctrl_heat;
        network_msg_ctrl_heat_set_t * ctrl_heat_set;
        network_msg_ctrl_layout_t * ctrl_layout;
        network_msg_ctrl_layout_set_t * ctrl_layout_set;
        network_msg_chlor_ping_req_t * chlor_ping_req;
        network_msg_chlor_ping_t * chlor_ping;
        network_msg_chlor_name_t * chlor_name;
        network_msg_chlor_level_set_t * chlor_level_set;
        network_msg_chlor_level_resp_t * chlor_level_resp;
    } u;
} network_msg_t;

/* network.c */
network_addrgroup_t network_groupaddr(uint16_t const addr);
uint8_t network_devaddr(uint8_t group, uint8_t const id);
bool network_rx_msg(datalink_pkt_t const * const datalink, network_msg_t * const network, bool * const txOpportunity);

/* network_str.c */
char const * network_date_str(uint8_t const year, uint8_t const month, uint8_t const day);
char const * network_time_str(uint8_t const hours, uint8_t const minutes);
char const * network_version_str(uint8_t const major, uint8_t const minor);
const char * network_msg_typ_str(network_msg_typ_t const typ);
const char * network_circuit_str(network_circuit_t const circuit);
const char * network_pump_mode_str(network_pump_mode_t const pump_mode);
char const * network_pump_prg_str(uint16_t const address);
const char * network_heat_src_str(network_heat_src_t const heat_src);
uint network_heat_src_nr(char const * const heat_src);
