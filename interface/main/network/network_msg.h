#pragma once
#include <esp_system.h>

// separated from network.h, to prevent #include loop

#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

/**
 * NETWORK_TYP_CTRL_t
 **/

// #define NETWORK_TYP_CTRL_SET (0x80)
// #define NETWORK_TYP_CTRL_REQ (0xC0)

#define NETWORK_TYP_CTRL_MAP(XX) \
  XX(0x01, SET_ACK)              \
  XX(0x02, STATE_BCAST)          \
  XX(0x05, TIME_RESP)       XX(0x85, TIME_SET)     XX(0xC5, TIME_REQ)    \
  XX(0x06, CIRCUIT_RESP)    XX(0x86, CIRCUIT_SET)  XX(0xC6, CIRCUIT_REQ) \
  XX(0x08, HEAT_RESP)       XX(0x88, HEAT_SET)     XX(0xC8, HEAT_REQ)    \
  XX(0x1E, SCHED_RESP)      XX(0x9E, SCHED_SET)    XX(0xDE, SCHED_REQ)   \
  XX(0x21, LAYOUT_RESP)     XX(0xA1, LAYOUT_SET)   XX(0xE1, LAYOUT_REQ)  \
  XX(0x0B, CIRC_NAMES_RESP) XX(0xCB, CIRC_NAMES_REQ) \
  XX(0x11, SCHEDS_RESP)     XX(0xD1, SCHEDS_REQ)     \
  XX(0x12, UNKN_D2_RESP)    XX(0xD2, UNKN_D2_REQ)    \
  XX(0x1D, VALVE_RESP)      XX(0xDD, VALVE_REQ)      \
  XX(0x22, SOLARPUMP_RESP)  XX(0xE2, SOLARPUMP_REQ)  \
  XX(0x23, DELAY_RESP)      XX(0xE3, DELAY_REQ)      \
  XX(0x28, HEAT_SETPT_RESP) XX(0xE8, HEAT_SETPT_REQ) \
  XX(0xFC, VERSION_RESP)    XX(0xFD, VERSION_REQ)

/* results of sending requests:
NETWORK_TYP_CTRL_UNKNOWNxD9 = 0xD9, // sending [], returns: [11 3C 00 3F 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00]
*/

typedef enum {
#define XX(num, name) NETWORK_TYP_CTRL_##name = num,
  NETWORK_TYP_CTRL_MAP(XX)
#undef XX
} network_typ_ctrl_t;

/**
 * NETWORK_TYP_PUMP_t
 **/

// FYI occasionally there is a src=0x10 dst=0x60 typ=0xFF with data=[0x80]; pump doesn't reply to it
#define NETWORK_TYP_PUMP_MAP(XX) \
  XX(0x01, REG) \
  XX(0x04, CTRL) \
  XX(0x05, MODE) \
  XX(0x06, RUN) \
  XX(0x07, STATUS) \
  XX(0xff, FF)

typedef enum {
#define XX(num, name) NETWORK_TYP_PUMP_##name = num,
  NETWORK_TYP_PUMP_MAP(XX)
#undef XX
} network_typ_pump_t;

/**
 * NETWORK_TYP_CHLOR_t
 **/

// FYI there is a 0x14, has dst=0x50 data=[0x00]
 #define NETWORK_TYP_CHLOR_MAP(XX) \
  XX(0x00, PING_REQ)               \
  XX(0x01, PING_RESP)              \
  XX(0x03, NAME_RESP)              \
  XX(0x11, LEVEL_SET)              \
  XX(0x12, LEVEL_RESP)             \
  XX(0x14, NAME_REQ)

typedef enum {
#define XX(num, name) NETWORK_TYP_CHLOR_##name = num,
  NETWORK_TYP_CHLOR_MAP(XX)
#undef XX
} network_typ_chlor_t;

/**
 * macro "magic" to get an enum and matching *_str functions (in *_str.c)
 **/

#define NETWORK_MODE_MAP(XX) \
  XX( 0, service) \
  XX( 1, UNKOWN_01) \
  XX( 2, tempInc) \
  XX( 3, freezeProt) \
  XX( 4, timeout) \
  XX( 5, COUNT)

typedef enum {
#define XX(num, name) NETWORK_MODE_##name = num,
  NETWORK_MODE_MAP(XX)
#undef XX
} network_mode_t;

// MUST add 1 for network messages (1-based)
#define NETWORK_CIRCUIT_MAP(XX) \
  XX( 0, spa)  \
  XX( 1, aux1) \
  XX( 2, aux2) \
  XX( 3, aux3) \
  XX( 4, ft1)  \
  XX( 5, pool) \
  XX( 6, ft2)  \
  XX( 7, ft3)  \
  XX( 8, ft4)  \
  XX( 9, COUNT)

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

#define NETWORK_PUMP_STATE_MAP(XX) \
  XX(0, OK) \
  XX(1, PRIMING)  \
  XX(2, RUNNING)  \
  XX(3, X03)  \
  XX(4, SYSPRIMING)

typedef enum {
#define XX(num, name) NETWORK_PUMP_STATE_##name = num,
  NETWORK_PUMP_STATE_MAP(XX)
#undef XX
} network_pump_state_t;

#define NETWORK_HEAT_SRC_MAP(XX) \
  XX(0, None)       \
  XX(1, Heater)     \
  XX(2, SolarPref) \
  XX(3, Solar)

typedef enum {
#define XX(num, name) NETWORK_HEAT_SRC_##name = num,
  NETWORK_HEAT_SRC_MAP(XX)
#undef XX
} network_heat_src_t;


/*
 * A5 messages, used to communicate with components except IntelliChlor
 */

typedef struct network_msg_none_t {
} network_msg_none_t;

typedef struct network_msg_ctrl_set_ack_t {
    uint8_t typ;  // NETWORK_TYP_CTRL_*  type that it is ACK'íng
} PACK8 network_msg_ctrl_set_ack_t;

typedef struct network_msg_ctrl_circuit_set_t {
    uint8_t circuit;  // 1-based
    uint8_t value;
} PACK8 network_msg_ctrl_circuit_set_t;

typedef struct network_msg_ctrl_sched_req_t {
} network_msg_ctrl_sched_req_t;

typedef struct network_msg_ctrl_sched_resp_sub_t {
    uint8_t circuit;            // 0  (0 = schedule not active)
    uint8_t UNKNOWN_1;          // 1
    uint8_t prgStartHi;         // 2 [min]
    uint8_t prgStartLo;         // 3 [min]
    uint8_t prgStopHi;          // 4 [min]
    uint8_t prgStopLo;          // 5 [min]
} PACK8 network_msg_ctrl_sched_resp_sub_t;

typedef struct network_msg_ctrl_sched_resp_t {
    uint8_t                           UNKNOWN_0to3[4];  // 0,1,2,3
    network_msg_ctrl_sched_resp_sub_t scheds[2];        // 4,5,6,7,8,9, 10,11,12,13,14,15
} PACK8 network_msg_ctrl_sched_resp_t;

#if 0
typedef struct network_msg_ctrl_state_req_t {
} network_msg_ctrl_state_req_t;
#endif

typedef struct network_msg_ctrl_state_bcast_t {
    uint8_t hour;               // 0
    uint8_t minute;             // 1
    uint8_t activeLo;           // 2
    uint8_t activeHi;           // 3
    uint8_t UNKNOWN_4to6[3];    // 4..6 more `active` circuits on fancy controllers
    uint8_t UNKNOWN_7to8[2];    // 7..8
    uint8_t modes;               // 9
    uint8_t heatStatus;         // 10
    uint8_t UNKNOWN_11;         // 11
    uint8_t delay;              // 12
    uint8_t UNKNOWN_13;         // 13
    uint8_t poolTemp;           // 14 water sensor 1
    uint8_t spaTemp;            // 15 water sensor 2
    uint8_t UNKNOWN_16;         // 16 unknown          (was major)
    uint8_t solarTemp;          // 17 solar sensor 1   (was minor)
    uint8_t airTemp;            // 18 air sensor
    uint8_t solarTemp2;         // 19 solar sensor 2
    uint8_t UNKNOWN_20tp21[2];  // 20..21 more water sensors?
    uint8_t heatSrc;            // 22 
    uint8_t UNKNOWN_23to28[6];  // 23..28
} PACK8 network_msg_ctrl_state_bcast_t;

// just a guess
typedef network_msg_ctrl_state_bcast_t network_msg_ctrl_state_set_t;

// TIME

typedef struct network_msg_ctrl_time_req_t {
} network_msg_ctrl_time_req_t;

// message from controller to all (NETWORK_TYP_CTRL_TIME, or NETWORK_TYP_CTRL_TIME_SET)
typedef struct network_msg_ctrl_time_t {
    uint8_t hour;            // 0
    uint8_t minute;          // 1
    uint8_t UNKNOWN_2;       // 2 (DST adjust?)
    uint8_t day;             // 3
    uint8_t month;           // 4
    uint8_t year;            // 5
    uint8_t clkSpeed;        // 6
    uint8_t daylightSavings; // 7 (1=auto, 0=manual)
} PACK8 network_msg_ctrl_time_t;

typedef network_msg_ctrl_time_t network_msg_ctrl_time_set_t;
typedef network_msg_ctrl_time_t network_msg_ctrl_time_resp_t;

// VERSION

typedef struct network_msg_ctrl_version_req_t {
//    uint8_t reqId;
} network_msg_ctrl_version_req_t;

typedef struct network_msg_ctrl_version_resp_t {
    uint8_t reqId;       // 0
    uint8_t major;       // 1    0x02  -> version 2.080
    uint8_t minor;       // 2    0x50
    uint8_t UNK3to4[2];  // 3,4
    uint8_t bootMajor;   // 5
    uint8_t bootMinor;   // 6   
    uint8_t U07to16[10]; // 7,8,9,10,11, 12,13,14,15,16
} network_msg_ctrl_version_resp_t;

// VALVE

typedef struct network_msg_ctrl_valve_req_t {
} network_msg_ctrl_valve_req_t;

typedef struct network_msg_ctrl_valve_resp_t {
    uint8_t UNKNOWN[24]; // 03 00 00 00 00 FF FF 01 02 03 04 01 48 00 00 00 03 00 00 00 04 00 00 00
} network_msg_ctrl_valve_resp_t;

// SOLARPUMP

typedef struct network_msg_ctrl_solarpump_req_t {
} network_msg_ctrl_solarpump_req_t;

typedef struct network_msg_ctrl_solarpump_resp_t {
    uint8_t UNKNOWN[3];  // 05 00 00
} network_msg_ctrl_solarpump_resp_t;

// DELAY_REQ

typedef struct network_msg_ctrl_delay_req_t {
} network_msg_ctrl_delay_req_t;

typedef struct network_msg_ctrl_delay_resp_t {
    uint8_t UNKNOWN[2];  // 10 00
} network_msg_ctrl_delay_resp_t;

// HEAT_SETPT

typedef struct network_msg_ctrl_heat_setpt_req_t {
} network_msg_ctrl_heat_setpt_req_t;

typedef struct network_msg_ctrl_heat_setpt_resp_t {
    uint8_t UNKNOWN[10];  // 00 00 00 00 00 00 00 00 00 00 
} network_msg_ctrl_heat_setpt_resp_t;

// CIRC_NAMES

typedef struct network_msg_ctrl_circ_names_req_t {
    uint8_t reqId;  // 0x01
} network_msg_ctrl_circ_names_req_t;

typedef struct network_msg_ctrl_circ_names_resp_t {
    uint8_t reqId;       // req 0x01 -> resp 01 01 48 00 00
    uint8_t UNKNOWN[5];  // req 0x02 -> resp 02 00 03 00 00
} network_msg_ctrl_circ_names_resp_t;

// UNKN_D2_REQ

typedef struct network_msg_ctrl_unkn_d2_req_t {
    uint8_t UNKNOWN;  // 0xD2
} network_msg_ctrl_unkn_d2_req_t;

// SCHEDS

typedef struct network_msg_ctrl_scheds_req_t {
    uint8_t schedId;  // 0x01 (1 - 12)
} network_msg_ctrl_scheds_req_t;

// With POOL 1/1 from 08:00 to 10:00, and
//      SPA  1/1 from 11:00 to 12:00
// sending [0] => no response
// sending [1] => 01 01 08 00 00 00 3F
// sending [2] => 02 01 0C 30 15 14 3F
// sending [3] => 03 00 2E 38 08 25 3F
//
// With no POOL, no SPA, no ..
// sending [1] => 01 00 00 00 00 00 3F
// sending [2] => 02 01 0C 30 15 14 3F
// sending [3] => 03 00 2E 38 08 25 3F
//
// With only POOL 1/1 from 08:00 to 10:00
// sending [1] => 01 06 00 00 00 00 3F
// sending [2] => 02 01 0C 30 15 14 3F
// sending [3] => 03 00 2E 38 08 25 3F
//
typedef struct network_msg_ctrl_scheds_resp_t {
    uint8_t schedId;  // 0 
    uint8_t circuit;  // 1
    uint8_t startHr;  // 2
    uint8_t startMin; // 3
    uint8_t stopHr;   // 4
    uint8_t stopMin;  // 5
    uint8_t dayOfWk;  // 6 bitmask Mon (0x01), Tue (0x02), Wed (0x04), Thu(0x08), Fri (0x10), Sat (0x20), Sun(0x40)
} network_msg_ctrl_scheds_resp_t;

// HEAT

typedef struct network_msg_ctrl_heat_req_t {
} network_msg_ctrl_heat_req_t;

typedef struct network_msg_ctrl_heat_resp_t {
    uint8_t poolTemp;      // 0
    uint8_t spaTemp;       // 1
    uint8_t airTemp;       // 2
    uint8_t poolSetpoint;  // 3
    uint8_t spaSetpoint;   // 4
    uint8_t heatSrc;       // 5
    uint8_t UNKNOWN_6;     // 6
    uint8_t UNKNOWN_7;     // 7
    uint8_t UNKNOWN_8;     // 8
    uint8_t UNKNOWN_9;     // 9
    uint8_t UNKNOWN_10;    // 10
    uint8_t UNKNOWN_11;    // 11
    uint8_t UNKNOWN_12;    // 12
} PACK8 network_msg_ctrl_heat_resp_t;

typedef struct network_msg_ctrl_heat_set_t {
    uint8_t poolSetpoint;  // 0
    uint8_t spaSetpoint;   // 1
    uint8_t heatSrc;       // 2
    uint8_t UNKNOWN_3;     // 3
} PACK8 network_msg_ctrl_heat_set_t;

// LAYOUT

typedef struct network_msg_ctrl_layout_req_t {
} network_msg_ctrl_layout_req_t;

typedef struct network_msg_ctrl_layout_resp_t {
    uint8_t circuit[4];  // circuits assigned to each of the 4 buttons on the remote
} PACK8 network_msg_ctrl_layout_resp_t;

typedef network_msg_ctrl_layout_resp_t network_msg_ctrl_layout_set_t;

// PUMP_REG

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

// PUMP CTRL

typedef struct network_msg_pump_ctrl_t {
    uint8_t ctrl;     // 0
} PACK8 network_msg_pump_ctrl_t;

typedef struct network_msg_pump_mode_t {
    uint8_t mode;        // 0
} PACK8 network_msg_pump_mode_t;

// PUMP RUN

typedef struct network_msg_pump_run_t {
    uint8_t running;       // 0
} PACK8 network_msg_pump_run_t;

// PUMP STATUS

typedef struct network_msg_pump_status_req_t {
} network_msg_pump_status_req_t;

typedef struct network_msg_pump_status_resp_t {
    uint8_t running;     // 0
    uint8_t mode;        // 1
    uint8_t state;       // 2
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
} PACK8 network_msg_pump_status_resp_t;

/*
* IC messages, use to communicate with IntelliChlor
*/

// CHLOR PING

typedef struct network_msg_chlor_ping_req_t {
    uint8_t UNKNOWN_0;
} PACK8 network_msg_chlor_ping_req_t;

typedef struct network_msg_chlor_ping_resp_t {
    uint8_t UNKNOWN_0;
    uint8_t UNKNOWN_1;
} PACK8 network_msg_chlor_ping_resp_t;

// CHLOR_NAME

typedef char network_msg_chlor_name_str_t[16];

typedef struct network_msg_chlor_name_req_t {
    uint8_t UNKNOWN;  // Sending 0x00 or 0x02 gets a response
} PACK8 network_msg_chlor_name_req_t;

typedef struct network_msg_chlor_name_resp_t {
    uint8_t                      salt;  // ppm/50
    network_msg_chlor_name_str_t name;
} PACK8 network_msg_chlor_name_resp_t;

// CHLOR_LEVEL_SET

typedef struct network_msg_chlor_level_set_t {
    uint8_t pct;
} PACK8 network_msg_chlor_level_set_t;

typedef struct network_msg_chlor_level_resp_t {
    uint8_t salt;  // ppm/50
    uint8_t err;   // error bits: low flow (0x01), low salt (0x02), high salt (0x04), clean cell (0x10), cold (0x40), OK (0x80)
} PACK8 network_msg_chlor_level_resp_t;

//

typedef union network_msg_data_a5_t {
    network_msg_pump_reg_set_t pump_reg_set;
    network_msg_pump_reg_resp_t pump_reg_set_resp;
    network_msg_pump_ctrl_t pump_ctrl;
    network_msg_pump_mode_t pump_mode;
    network_msg_pump_run_t pump_run;
    network_msg_pump_status_resp_t pump_status;
    network_msg_ctrl_set_ack_t ctrl_set_ack;
    network_msg_ctrl_circuit_set_t ctrl_circuit_set;
    network_msg_ctrl_sched_resp_t ctrl_sched;
    network_msg_ctrl_state_bcast_t ctrl_state;
    network_msg_ctrl_state_set_t ctrl_state_set;
    network_msg_ctrl_time_resp_t ctrl_time_resp;
    network_msg_ctrl_heat_resp_t ctrl_heat_resp;
    network_msg_ctrl_heat_set_t ctrl_heat_set;
    network_msg_ctrl_layout_resp_t ctrl_layout_resp;
    network_msg_ctrl_layout_set_t ctrl_layout_set;    
    network_msg_ctrl_version_req_t ctrl_version_req;
    network_msg_ctrl_version_resp_t ctrl_version_resp;
    network_msg_ctrl_valve_req_t ctrl_valve_req;
    network_msg_ctrl_valve_resp_t ctrl_valve_resp;
    network_msg_ctrl_solarpump_req_t ctrl_solarpump_req;
    network_msg_ctrl_solarpump_resp_t ctrl_solarpump_resp;
    network_msg_ctrl_delay_req_t ctrl_delay_req;
    network_msg_ctrl_delay_resp_t ctrl_delay_resp;
    network_msg_ctrl_heat_setpt_req_t ctrl_heat_setpt_req;
    network_msg_ctrl_heat_setpt_resp_t ctrl_heat_setpt_resp;
    network_msg_ctrl_circ_names_req_t ctrl_circ_names_req;
    network_msg_ctrl_circ_names_resp_t ctrl_circ_names_resp;
    network_msg_ctrl_unkn_d2_req_t ctrl_unkn_d2_req;
    network_msg_ctrl_scheds_req_t ctrl_scheds_req;
    network_msg_ctrl_scheds_resp_t ctrl_scheds_resp;
} PACK8 network_msg_data_a5_t;

typedef union network_msg_data_ic_t {
    network_msg_chlor_ping_req_t   chlor_ping_req;
    network_msg_chlor_ping_resp_t  chlor_ping;
    network_msg_chlor_name_req_t   chlor_name_req;
    network_msg_chlor_name_resp_t  chlor_name_resp;
    network_msg_chlor_level_set_t  chlor_level_set;
    network_msg_chlor_level_resp_t chlor_level_resp;
} PACK8 network_msg_data_ic_t;

typedef union network_msg_data_t {
    network_msg_data_a5_t a5;
    network_msg_data_ic_t ic;
} PACK8 network_msg_data_t;
#define NETWORK_DATA_MAX_SIZE (sizeof(network_msg_data_t))

#define NETWORK_MSG_TYP_MAP(XX) \
  XX( 0, NONE,             network_msg_none_t,             0,                     0)                             \
  XX( 1, CTRL_SET_ACK,     network_msg_ctrl_set_ack_t,     DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SET_ACK)     \
  XX( 2, CTRL_CIRCUIT_SET, network_msg_ctrl_circuit_set_t, DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_CIRCUIT_SET) \
  XX( 3, CTRL_SCHED_REQ,   network_msg_ctrl_sched_req_t,   DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SCHED_REQ)   \
  XX( 4, CTRL_SCHED_RESP,  network_msg_ctrl_sched_resp_t,  DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SCHED_RESP)  \
  XX( 5, CTRL_STATE_BCAST, network_msg_ctrl_state_bcast_t, DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_STATE_BCAST) \
  XX( 6, CTRL_TIME_REQ,    network_msg_ctrl_time_req_t,    DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_TIME_REQ)    \
  XX( 7, CTRL_TIME_RESP,   network_msg_ctrl_time_resp_t,   DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_TIME_RESP)   \
  XX( 8, CTRL_TIME_SET,    network_msg_ctrl_time_set_t,    DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_TIME_SET)    \
  XX( 9, CTRL_HEAT_REQ,    network_msg_ctrl_heat_req_t,    DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_HEAT_REQ)    \
  XX(10, CTRL_HEAT_RESP,   network_msg_ctrl_heat_resp_t,   DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_HEAT_RESP)   \
  XX(11, CTRL_HEAT_SET,    network_msg_ctrl_heat_set_t,    DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_HEAT_SET)    \
  XX(12, CTRL_LAYOUT_REQ,  network_msg_ctrl_layout_req_t,  DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_LAYOUT_REQ)  \
  XX(13, CTRL_LAYOUT,      network_msg_ctrl_layout_resp_t, DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_LAYOUT_RESP) \
  XX(14, CTRL_LAYOUT_SET,  network_msg_ctrl_layout_set_t,  DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_LAYOUT_SET)  \
  XX(15, PUMP_REG_SET,     network_msg_pump_reg_set_t,     DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_REG)         \
  XX(16, PUMP_REG_RESP,    network_msg_pump_reg_resp_t,    DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_REG)         \
  XX(17, PUMP_CTRL_SET,    network_msg_pump_ctrl_t,        DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_CTRL)        \
  XX(18, PUMP_CTRL_RESP,   network_msg_pump_ctrl_t,        DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_CTRL)        \
  XX(19, PUMP_MODE_SET,    network_msg_pump_mode_t,        DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_MODE)        \
  XX(20, PUMP_MODE_RESP,   network_msg_pump_mode_t,        DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_MODE)        \
  XX(21, PUMP_RUN_SET,     network_msg_pump_run_t,         DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_RUN)         \
  XX(22, PUMP_RUN_RESP,    network_msg_pump_run_t,         DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_RUN)         \
  XX(23, PUMP_STATUS_REQ,  network_msg_pump_status_req_t,  DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_STATUS)      \
  XX(24, PUMP_STATUS_RESP, network_msg_pump_status_resp_t, DATALINK_PROT_A5_PUMP, NETWORK_TYP_PUMP_STATUS)      \
  XX(25, CHLOR_PING_REQ,   network_msg_chlor_ping_req_t,   DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_PING_REQ)   \
  XX(26, CHLOR_PING_RESP,  network_msg_chlor_ping_resp_t,  DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_PING_RESP)  \
  XX(27, CHLOR_NAME_RESP,  network_msg_chlor_name_resp_t,  DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_NAME_RESP) \
  XX(28, CHLOR_LEVEL_SET,  network_msg_chlor_level_set_t,  DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_LEVEL_SET)  \
  XX(29, CHLOR_LEVEL_RESP, network_msg_chlor_level_resp_t, DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_LEVEL_RESP) \
  XX(30, CHLOR_NAME_REQ,   network_msg_chlor_name_req_t,   DATALINK_PROT_IC,      NETWORK_TYP_CHLOR_NAME_REQ)    \
  XX(31, CTRL_VALVE_REQ,       network_msg_ctrl_valve_req_t,       DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_VALVE_REQ)       \
  XX(32, CTRL_VALVE_RESP,      network_msg_ctrl_valve_resp_t,      DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_VALVE_RESP)      \
  XX(33, CTRL_VERSION_REQ,     network_msg_ctrl_version_req_t,     DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_VERSION_REQ)     \
  XX(34, CTRL_VERSION_RESP,    network_msg_ctrl_version_resp_t,    DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_VERSION_RESP)    \
  XX(35, CTRL_SOLARPUMP_REQ,   network_msg_ctrl_solarpump_req_t,   DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SOLARPUMP_REQ)   \
  XX(36, CTRL_SOLARPUMP_RESP,  network_msg_ctrl_solarpump_resp_t,  DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SOLARPUMP_RESP)  \
  XX(37, CTRL_DELAY_REQ,       network_msg_ctrl_delay_req_t,       DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_DELAY_REQ)       \
  XX(38, CTRL_DELAY_RESP,      network_msg_ctrl_delay_resp_t,      DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_DELAY_RESP)      \
  XX(39, CTRL_HEAT_SETPT_REQ,  network_msg_ctrl_heat_setpt_req_t,  DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_HEAT_SETPT_REQ)  \
  XX(40, CTRL_HEAT_SETPT_RESP, network_msg_ctrl_heat_setpt_resp_t, DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_HEAT_SETPT_RESP) \
  XX(41, CTRL_CIRC_NAMES_REQ,  network_msg_ctrl_circ_names_req_t,  DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_CIRC_NAMES_REQ)  \
  XX(42, CTRL_CIRC_NAMES_RESP, network_msg_ctrl_circ_names_resp_t, DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_CIRC_NAMES_RESP) \
  XX(43, CTRL_SCHEDS_REQ,      network_msg_ctrl_scheds_req_t,      DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SCHEDS_REQ)      \
  XX(44, CTRL_SCHEDS_RESP,     network_msg_ctrl_scheds_resp_t,     DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_SCHEDS_RESP)     \
  XX(45, CTRL_UNKN_D2_REQ,     network_msg_ctrl_unkn_d2_req_t,     DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_UNKN_D2_REQ)
  
typedef enum {
#define XX(num, name, typ, proto, prot_typ) MSG_TYP_##name = num,
  NETWORK_MSG_TYP_MAP(XX)
#undef XX
} network_msg_typ_t;

typedef struct network_msg_t {
    network_msg_typ_t typ;
    union network_msg_union_t {
        network_msg_pump_reg_set_t * pump_reg_set;
        network_msg_pump_reg_resp_t * pump_reg_set_resp;
        network_msg_pump_ctrl_t * pump_ctrl;
        network_msg_pump_mode_t * pump_mode;
        network_msg_pump_run_t * pump_run;
        network_msg_pump_status_resp_t * pump_status_resp;
        network_msg_ctrl_set_ack_t * ctrl_set_ack;
        network_msg_ctrl_circuit_set_t * ctrl_circuit_set;
        network_msg_ctrl_sched_resp_t * ctrl_sched_resp;
        network_msg_ctrl_state_bcast_t * ctrl_state;
        network_msg_ctrl_time_set_t * ctrl_time_set;
        network_msg_ctrl_time_resp_t * ctrl_time_resp;
        network_msg_ctrl_heat_resp_t * ctrl_heat_resp;
        network_msg_ctrl_heat_set_t * ctrl_heat_set;
        network_msg_ctrl_layout_resp_t * ctrl_layout_resp;
        network_msg_ctrl_layout_set_t * ctrl_layout_set;
        network_msg_ctrl_version_req_t * ctrl_version_req;
        network_msg_ctrl_version_resp_t * ctrl_version_resp;
        network_msg_ctrl_valve_req_t * ctrl_valve_req;
        network_msg_ctrl_valve_resp_t * ctrl_valve_resp;
        network_msg_ctrl_solarpump_req_t * ctrl_solarpump_req;
        network_msg_ctrl_solarpump_resp_t * ctrl_solarpump_resp;
        network_msg_ctrl_delay_req_t * ctrl_delay_req;
        network_msg_ctrl_delay_resp_t * ctrl_delay_resp;
        network_msg_ctrl_heat_setpt_req_t * ctrl_heat_set_req;
        network_msg_ctrl_heat_setpt_resp_t * ctrl_heat_set_resp;
        network_msg_ctrl_circ_names_req_t * ctrl_circ_names_req;
        network_msg_ctrl_circ_names_resp_t * ctrl_circ_names_resp;
        network_msg_ctrl_unkn_d2_req_t * ctrl_unkn_d2_req;
        network_msg_ctrl_scheds_req_t * ctrl_scheds_req;
        network_msg_ctrl_scheds_resp_t * ctrl_scheds_resp;
        network_msg_chlor_ping_req_t * chlor_ping_req;
        network_msg_chlor_ping_resp_t * chlor_ping;
        network_msg_chlor_name_resp_t * chlor_name_resp;
        network_msg_chlor_level_set_t * chlor_level_set;
        network_msg_chlor_level_resp_t * chlor_level_resp;
        network_msg_chlor_name_req_t * chlor_name_req;
        uint8_t * bytes;
    } u;
} network_msg_t;
