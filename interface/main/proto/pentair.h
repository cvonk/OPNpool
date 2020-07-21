#pragma once

#include <esp_system.h>

// struct/emum mapping
#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

typedef enum {
    PROTOCOL_A5 = 0,
    PROTOCOL_IC,
} PROTOCOL_t;
#define PROTOCOL_COUNT (2)

typedef enum {
    ADDRGROUP_ALL = 0x00,
    ADDRGROUP_CTRL = 0x01,
    ADDRGROUP_REMOTE = 0x02,
    ADDRGROUP_CHLOR = 0x05,
    ADDRGROUP_PUMP = 0x06,
    ADDRGROUP_COUNT = 0x10,
} ADDRGROUP_t;

/* results of sending requests:
MT_CTRL_UNKNOWNxCB = 0xCB, // sending [],   returns: 01 01 48 00 00
MT_CTRL_UNKNOWNxD1 = 0xD1, // sending [],   returns: 01 06 00 00 00 00 3F
MT_CTRL_UNKNOWNxD9 = 0xD9, // sending [],   returns: 11 3C 00 3F 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
MT_CTRL_UNKNOWNxDD = 0xDD, // sending [],   returns: 03 00 00 00 00 FF FF 01 02 03 04 01 48 00 00 00 03 00 00 00 04 00 00 00
MT_CTRL_UNKNOWNxE2 = 0xE2, // sending [],   returns: 05 00 00 => looks like a boring ic ping request
MT_CTRL_UNKNOWNxE3 = 0xE3, // sending [],   returns: 10 00
MT_CTRL_UNKNOWNxE8 = 0xE8, // sending [],   returns: 00 00 00 00 00 00 00 00 00 00
MT_CTRL_UNKNOWN_FD = 0xFD, // sending [],   returns: 01 02 50 00 00 00 00 00 00 00 00 00 00 00 00 00 00
*/

// #define MT_CTRL_SET (0x80)
// #define MT_CTRL_REQ (0xC0)

typedef enum MT_CTRL_A5_t {
    MT_CTRL_SET_ACK = 0x01,
    MT_CTRL_CIRCUIT_SET = 0x86,
    MT_CTRL_STATE= 0x02,
    MT_CTRL_STATE_SET = 0x82, // (MT_CTRL_STATE| MT_CTRL_SET),
    MT_CTRL_STATE_REQ = 0xC2, //(MT_CTRL_STATE| MT_CTRL_REQ),
    MT_CTRL_TIME = 0x05,
    MT_CTRL_TIME_SET = 0x85, //(MT_CTRL_TIME | MT_CTRL_SET),
    MT_CTRL_TIME_REQ = 0xC5, //(MT_CTRL_TIME | MT_CTRL_REQ),
    MT_CTRL_HEAT = 0x08,
    MT_CTRL_HEAT_SET = 0x88, //(MT_CTRL_HEAT | MT_CTRL_SET),
    MT_CTRL_HEAT_REQ = 0xC8, //(MT_CTRL_HEAT | MT_CTRL_REQ),
    MT_CTRL_SCHED = 0x1E,
    //MT_CTRL_SCHED_SET = 0x9E, //(MT_CTRL_SCHED | MT_CTRL_SET),
    MT_CTRL_SCHED_REQ = 0xDE, //(MT_CTRL_SCHED | MT_CTRL_REQ),
#if 0
    MT_CTRL_LAYOUT = 0x21,
    MT_CTRL_LAYOUT_SET = (MT_CTRL_LAYOUT | MT_CTRL_SET),
    MT_CTRL_LAYOUT_REQ = (MT_CTRL_LAYOUT | MT_CTRL_REQ),
#endif
} MT_CTRL_A5_t;

typedef enum MT_PUMP_A5_t {
    MT_PUMP_REGULATE = 0x01,
    MT_PUMP_CTRL = 0x04,
    MT_PUMP_MODE = 0x05,
    MT_PUMP_STATE = 0x06,
    MT_PUMP_STATUS = 0x07,
    // FYI occasionally there is a src=0x10 dst=0x60 typ=0xFF with data=[0x80]; pump doesn't reply to it
    MT_PUMP_0xFF = 0xFF, // has src=0x10 dst=0x60 data=[0x08]
} MT_PUMP_A5_t;

typedef enum MT_CHLOR_IC_t {
    MT_CHLOR_PING_REQ = 0x00,
    MT_CHLOR_PING = 0x01,
    MT_CHLOR_NAME = 0x03,
    MT_CHLOR_LVLSET = 0x11,
    MT_CHLOR_LVLSET_RESP = 0x12,
    MT_CHLOR_0x14 = 0x14, // has dst=0x50 data=[0x00]
} MT_CHLOR_IC_t;

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

typedef struct mHdr_a5_t {
    uint8_t pro;  // protocol version
    uint8_t dst;  // destination
    uint8_t src;  // source
    uint8_t typ;  // message type
    uint8_t len;  // # of data bytes following
} PACK8 mHdr_a5_t;

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
    uint8_t airtemp;            // 18
    uint8_t solartemp;          // 19
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
    mCtrlSchedSub_a5_t sched[2];  // 4,5,6,7,8,9, 10,11,12,13,14,15
} PACK8 mCtrlSched_a5_t;

typedef struct mCtrlLayout_a5_t {
    uint8_t circuit[4];  // 0..3 corresponding to the buttons on the remote
} PACK8 mCtrlLayout_a5_t;

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

typedef struct mPumpControl_a5_t {
    uint8_t control;     // 0
} PACK8 mPumpControl_a5_t;

typedef struct mPumpMode_a5_t {
    uint8_t mode;        // 0
} PACK8 mPumpMode_a5_t;

typedef struct mPumpState_a5_t {
    uint8_t state;       // 0
} PACK8 mPumpState_a5_t;

typedef struct mPumpStatus_a5_t {
    uint8_t state;       // 0
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

typedef struct mPump0xFF_a5_t {
    uint8_t UNKNOWN_0;
} PACK8 mPump0xFF_a5_t;

/*
* IC messages, use to communicate with IntelliChlor
*/

typedef struct mHdr_ic_t {
    uint8_t dst;  // destination
    uint8_t typ;  // message type
} PACK8 mHdr_ic_t;

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

typedef struct mChlorLvlSet_ic_t {
    uint8_t pct;
} PACK8 mChlorLvlSet_ic_t;

typedef struct mChlorLvlSetResp_ic_t {
    uint8_t salt;
    uint8_t err;  // call ; cold water
} PACK8 mChlorLvlSetResp_ic_t;

typedef struct mChlor0X14_ic_t {
    uint8_t UNKNOWN_0;
} PACK8 mChlor0X14_ic_t;

typedef struct pentairMsg_t {
	PROTOCOL_t  proto;
	mHdr_a5_t   hdr;
	uint8_t     data[CONFIG_POOL_MSGDATA_BUFSIZE];
    uint16_t    chk;
} pentairMsg_t;
