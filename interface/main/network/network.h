#pragma once
#include <esp_system.h>

#include "../datalink/datalink.h"

// struct/emum mapping
#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

/* results of sending requests:
DATALINK_A5_CTRL_MSGTYP_UNKNOWNxCB = 0xCB, // sending [],   returns: 01 01 48 00 00
DATALINK_A5_CTRL_MSGTYP_UNKNOWNxD1 = 0xD1, // sending [],   returns: 01 06 00 00 00 00 3F
DATALINK_A5_CTRL_MSGTYP_UNKNOWNxD9 = 0xD9, // sending [],   returns: 11 3C 00 3F 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
DATALINK_A5_CTRL_MSGTYP_UNKNOWNxDD = 0xDD, // sending [],   returns: 03 00 00 00 00 FF FF 01 02 03 04 01 48 00 00 00 03 00 00 00 04 00 00 00
DATALINK_A5_CTRL_MSGTYP_UNKNOWNxE2 = 0xE2, // sending [],   returns: 05 00 00 => looks like a boring ic ping request
DATALINK_A5_CTRL_MSGTYP_UNKNOWNxE3 = 0xE3, // sending [],   returns: 10 00
DATALINK_A5_CTRL_MSGTYP_UNKNOWNxE8 = 0xE8, // sending [],   returns: 00 00 00 00 00 00 00 00 00 00
DATALINK_A5_CTRL_MSGTYP_UNKNOWN_FD = 0xFD, // sending [],   returns: 01 02 50 00 00 00 00 00 00 00 00 00 00 00 00 00 00
*/

typedef enum CHLORSTATE_t {
    CHLORSTATE_OK,
    CHLORSTATE_HIGH_SALT,
    CHLORSTATE_LOW_SALT,
    CHLORSTATE_VERYLOW_SALT,
    CHLORSTATE_LOW_FLOW,
} CHLORSTATE_t;

typedef enum CIRCUITNR_t {
    CIRCUITNR_SPA = 1,
    CIRCUITNR_AUX1,
    CIRCUITNR_AUX2,
    CIRCUITNR_AUX3,
    CIRCUITNR_FT1,
    CIRCUITNR_POOL,
    CIRCUITNR_FT2,
    CIRCUITNR_FT3,
    CIRCUITNR_FT4
} CIRCUITNR_t;

/*
 * A5 messages, used to communicate with components except IntelliChlor
 */

typedef struct mCtrlSetAck_a5_t {
    uint8_t typ;
} PACK8 mCtrlSetAck_a5_t;

typedef struct mCtrlState_a5_t {
    uint8_t hour;               // 0
    uint8_t minute;             // 1
    uint8_t activeLo;           // 2
    uint8_t activeHi;           // 3
    uint8_t UNKNOWN_4to8[5];    // 4..8
    uint8_t remote;             // 9
    uint8_t heating;            // 10
    uint8_t UNKNOWN_11;         // 11
    uint8_t delayLo;            // 12
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
} PACK8 mCtrlState_a5_t;

typedef struct mCtrlTime_a5_t {
    uint8_t hour;            // 0
    uint8_t minute;          // 1
    uint8_t UNKNOWN_2;        // 2 (DST adjust?)
    uint8_t day;             // 3
    uint8_t month;           // 4
    uint8_t year;            // 5
    uint8_t clkSpeed;        // 6
    uint8_t daylightSavings; // 7
} PACK8 mCtrlTime_a5_t;

typedef mCtrlTime_a5_t mCtrlTimeSet_a5_t;

typedef struct mCtrlHeat_a5_t {
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
} PACK8 mCtrlHeat_a5_t;

typedef struct mCtrlHeatSet_a5_t {
    uint8_t poolTempSetpoint;  // 0
    uint8_t spaTempSetpoint;   // 1
    uint8_t heatSrc;           // 2
    uint8_t UNKNOWN_3;         // 3
} PACK8 mCtrlHeatSet_a5_t;

typedef struct mCtrlCircuitSet_a5_t {
    uint8_t circuit;
    uint8_t value;
} PACK8 mCtrlCircuitSet_a5_t;

typedef struct mCtrlSchedSub_a5_t {
    uint8_t circuit;            // 0
    uint8_t UNKNOWN_1;          // 1
    uint8_t prgStartHi;         // 2 [min]
    uint8_t prgStartLo;         // 3 [min]
    uint8_t prgStopHi;          // 4 [min]
    uint8_t prgStopLo;          // 5 [min]
} PACK8 mCtrlSchedSub_a5_t;

typedef struct mCtrlSched_a5_t {
    uint8_t UNKNOWN_0to3[4];      // 0,1,2,3
    mCtrlSchedSub_a5_t scheds[2];  // 4,5,6,7,8,9, 10,11,12,13,14,15
} PACK8 mCtrlSched_a5_t;

typedef struct mCtrlLayout_a5_t {
    uint8_t circuit[4];  // 0..3 corresponding to the buttons on the remote
} PACK8 mCtrlLayout_a5_t;

typedef mCtrlLayout_a5_t mCtrlLayoutSet_a5_t;

typedef struct mPumpRegulateSet_a5_t {
    uint8_t addressHi;   // 0
    uint8_t addressLo;   // 1
    uint8_t valueHi;     // 2
    uint8_t valueLo;     // 3
} PACK8 mPumpRegulateSet_a5_t;

typedef struct mPumpRegulateSetResp_a5_t {
    uint8_t valueHi;     // 0
    uint8_t valueLo;     // 1
} PACK8 mPumpRegulateSetResp_a5_t;

typedef struct mPumpCtrl_a5_t {
    uint8_t ctrl;     // 0
} PACK8 mPumpCtrl_a5_t;

typedef struct mPumpMode_a5_t {
    uint8_t mode;        // 0
} PACK8 mPumpMode_a5_t;

typedef struct mPumpRunning_a5_t {
    uint8_t running;       // 0
} PACK8 mPumpRunning_a5_t;

typedef struct mPumpStatus_a5_t {
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
} PACK8 mPumpStatus_a5_t;

/*
* IC messages, use to communicate with IntelliChlor
*/

typedef struct mChlorPingReq_ic_t {
    uint8_t UNKNOWN_0;
} PACK8 mChlorPingReq_ic_t;

typedef struct mChlorPing_ic_t {
    uint8_t UNKNOWN_0;
    uint8_t UNKNOWN_1;
} PACK8 mChlorPing_ic_t;

typedef struct mChlorName_ic_t {
    uint8_t UNKNOWN_0;
    char name[16];
} PACK8 mChlorName_ic_t;

typedef struct mChlorLevelSet_ic_t {
    uint8_t pct;
} PACK8 mChlorLevelSet_ic_t;

typedef struct mChlorLevelResp_ic_t {
    uint8_t salt;
    uint8_t err;  // call ; cold water
} PACK8 mChlorLevelResp_ic_t;

typedef struct mChlor0X14_ic_t {
    uint8_t UNKNOWN_0;
} PACK8 mChlor0X14_ic_t;

typedef enum {
    NETWORK_ADDRGROUP_ALL = 0x00,
    NETWORK_ADDRGROUP_CTRL = 0x01,
    NETWORK_ADDRGROUP_REMOTE = 0x02,
    NETWORK_ADDRGROUP_CHLOR = 0x05,
    NETWORK_ADDRGROUP_PUMP = 0x06,
    NETWORK_ADDRGROUP_UNUSED9 = 0x09,
} NETWORK_ADDRGROUP_t;

// use macro "magic" to get an enum and matching name_* function (in name.c)
#define NETWORK_MSGTYP_MAP(XX) \
  XX( 0, NONE)              \
  XX( 1, PUMP_REG_SET)      \
  XX( 2, PUMP_REG_SET_RESP) \
  XX( 3, PUMP_CTRL)         \
  XX( 4, PUMP_MODE)         \
  XX( 5, PUMP_RUNNING)      \
  XX( 6, PUMP_STATUS_REQ)   \
  XX( 7, PUMP_STATUS)       \
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
#define XX(num, name) NETWORK_MSGTYP_##name = num,
  NETWORK_MSGTYP_MAP(XX)
#undef XX
} NETWORK_MSGTYP_t;

typedef struct network_msg_t {
    NETWORK_MSGTYP_t typ;
    union {
        // poolstate_t poolstate;
        mPumpRegulateSet_a5_t * pump_reg_set;
        mPumpRegulateSetResp_a5_t * pump_reg_set_resp;
        mPumpCtrl_a5_t * pump_ctrl;
        mPumpMode_a5_t * pump_mode;
        mPumpRunning_a5_t * pump_running;
        mPumpStatus_a5_t * pump_status;
        mCtrlSetAck_a5_t * ctrl_set_ack;
        mCtrlCircuitSet_a5_t * ctrl_circuit_set;
        mCtrlSched_a5_t * ctrl_sched;
        mCtrlState_a5_t * ctrl_state;
        mCtrlTime_a5_t * ctrl_time;
        mCtrlTimeSet_a5_t * ctrl_time_set;
        mCtrlHeat_a5_t * ctrl_heat;
        mCtrlHeatSet_a5_t * ctrl_heat_set;
        mCtrlLayout_a5_t * ctrl_layout;
        mCtrlLayoutSet_a5_t * ctrl_layout_set;
        mChlorPingReq_ic_t * chlor_ping_req;
        mChlorPing_ic_t * chlor_ping;
        mChlorName_ic_t * chlor_name;
        mChlorLevelSet_ic_t * chlor_level_set;
        mChlorLevelResp_ic_t * chlor_level_resp;
    } u;
} network_msg_t;

NETWORK_ADDRGROUP_t network_group_addr(uint16_t const addr);
uint8_t network_dev_addr(uint8_t group, uint8_t const id);

bool network_receive_msg(datalink_pkt_t const * const datalink, network_msg_t * const network, bool * const txOpportunity);
