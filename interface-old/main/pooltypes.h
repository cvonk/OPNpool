#pragma once
#include "sdkconfig.h"

// struct/emum mapping
#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

typedef enum {
    RESULT_done,
    RESULT_incomplete,
    RESULT_error
} result_t;

typedef enum {
    PROTOCOL_a5 = 0,
    PROTOCOL_ic
} protocol_t;
uint_least8_t const kProtocolCount = 2;

typedef enum {
    ADDRGROUP_all = 0x00,
    ADDRGROUP_ctrl = 0x01,
    ADDRGROUP_remote = 0x02,
    ADDRGROUP_unused3 = 0x03,
    ADDRGROUP_unused4 = 0x04,
    ADDRGROUP_chlor = 0x05,
    ADDRGROUP_pump = 0x06,
    ADDRGROUP_unused7 = 0x07,
    ADDRGROUP_unused8 = 0x08,
    ADDRGROUP_unused9 = 0x09,
    ADDRGROUP_COUNT
} addrGroup_t;

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

uint8_t const MT_CTRL_SET = 0x80;
uint8_t const MT_CTRL_REQ = 0xC0;

typedef enum {
    MT_CTRL_setAck = 0x01,
    MT_CTRL_circuitSet = 0x86,
    MT_CTRL_state = 0x02,
    MT_CTRL_stateSet = (MT_CTRL_state | MT_CTRL_SET),
    MT_CTRL_stateReq = (MT_CTRL_state | MT_CTRL_REQ),
    MT_CTRL_time = 0x05,
    MT_CTRL_timeSet = (MT_CTRL_time | MT_CTRL_SET),
    MT_CTRL_timeReq = (MT_CTRL_time | MT_CTRL_REQ),
    MT_CTRL_heat = 0x08,
    MT_CTRL_heatSet = (MT_CTRL_heat | MT_CTRL_SET),
    MT_CTRL_heatReq = (MT_CTRL_heat | MT_CTRL_REQ),
    MT_CTRL_sched = 0x1E,
    //MT_CTRL_schedSet = (MT_CTRL_sched | MT_CTRL_SET),
    MT_CTRL_schedReq = (MT_CTRL_sched | MT_CTRL_REQ),
#if 0
    MT_CTRL_layout = 0x21,
    MT_CTRL_layoutSet = (MT_CTRL_layout | MT_CTRL_SET),
    MT_CTRL_layoutReq = (MT_CTRL_layout | MT_CTRL_REQ)
#endif
} MT_CTRL_a5_t;

typedef enum {
    MT_PUMP_regulate = 0x01,
    MT_PUMP_control = 0x04,
    MT_PUMP_mode = 0x05,
    MT_PUMP_state = 0x06,
    MT_PUMP_status = 0x07,
    // FYI occasionally there is a src=0x10 dst=0x60 typ=0xFF with data=[0x80]; pump doesn't reply to it
    MT_PUMP_0xFF = 0xFF // has src=0x10 dst=0x60 data=[0x08]
} MT_PUMP_a5_t;

typedef enum {
    MT_CHLOR_pingReq = 0x00,
    MT_CHLOR_ping = 0x01,
    MT_CHLOR_name = 0x03,
    MT_CHLOR_lvlSet = 0x11,
    MT_CHLOR_lvlSetResp = 0x12,
    MT_CHLOR_0x14 = 0x14 // has dst=0x50 data=[0x00]
} MT_CHLOR_ic_t;

typedef enum {
    CHLORSTATE_ok,
    CHLORSTATE_highSalt,
    CHLORSTATE_lowSalt,
    CHLORSTATE_veryLowSalt,
    CHLORSTATE_lowFlow
} chlorState_t;

typedef enum {
    CIRCUITNR_spa = 1,
    CIRCUITNR_aux1,
    CIRCUITNR_aux2,
    CIRCUITNR_aux3,
    CIRCUITNR_ft1,
    CIRCUITNR_pool,
    CIRCUITNR_ft2,
    CIRCUITNR_ft3,
    CIRCUITNR_ft4
} circuit_t;


/*
* state, updated based on messages
*/

typedef struct {
    uint16_t active;
    uint16_t delay;
}  poolStateCircuits_t;

typedef struct {
    uint8_t temp;
    uint8_t setPoint;
    uint8_t heatSrc;
    bool heating;
} poolStateHeat_t;

typedef struct {
    uint8_t temp;
} poolStateAir_t;

typedef struct {
    uint8_t hour;
    uint8_t minute;
} poolStateTime_t ;

typedef struct {
    uint8_t day;
    uint8_t month;
    uint8_t year;
} poolStateDate_t;

typedef struct {
    uint8_t  circuit;
    uint16_t start;
    uint16_t stop;
} poolStateSched_t;

typedef struct {
    bool     running;
    uint8_t  mode;
    uint8_t  status;
    uint16_t pwr;
    uint16_t rpm;
    uint8_t  err;
} poolStatePump_t;

typedef struct {
    uint8_t pct;
    uint16_t salt;  // <2800 == low, <2600 == veryLow, > 4500 == high
    chlorState_t state;
} poolStateChlor_t;

typedef struct {
    poolStateTime_t     time;
    poolStateDate_t     date;
    poolStateHeat_t     pool, spa;
    poolStateAir_t      air;
    poolStateCircuits_t circuits;
    poolStateSched_t    sched[2];
    poolStatePump_t     pump;
    poolStateChlor_t    chlor;
} sysState_t;


/*
* A5 messages, used to communicate with components except IntelliChlor
*/

typedef struct {
    uint8_t pro;  // protocol version
    uint8_t dst;  // destination
    uint8_t src;  // source
    uint8_t typ;  // message type
    uint8_t len;  // # of data bytes following
} PACK8 mHdr_a5_t;

typedef struct {
    uint8_t typ;
} PACK8 mCtrlSetAck_a5_t;

typedef struct {
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

typedef struct {
    uint8_t hour;            // 0
    uint8_t minute;          // 1
    uint8_t UNKNOWN_2;        // 2 (DST adjust?)
    uint8_t day;             // 3
    uint8_t month;           // 4
    uint8_t year;            // 5
    uint8_t clkSpeed;        // 6
    uint8_t daylightSavings; // 7
} PACK8 mCtrlTime_a5_t;

typedef struct {
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

typedef struct {
    uint8_t poolTempSetpoint;  // 0
    uint8_t spaTempSetpoint;   // 1
    uint8_t heatSrc;           // 2
    uint8_t UNKNOWN_3;         // 3
} PACK8 mCtrlHeatSet_a5_t;

typedef struct {
    uint8_t circuit;
    uint8_t value;
} PACK8 mCtrlCircuitSet_a5_t;

typedef struct {
    uint8_t circuit;            // 0
    uint8_t UNKNOWN_1;          // 1
    uint8_t prgStartHi;         // 2 [min]
    uint8_t prgStartLo;         // 3 [min]
    uint8_t prgStopHi;          // 4 [min]
    uint8_t prgStopLo;          // 5 [min]
} PACK8 mCtrlSchedSub_a5_t;

typedef struct {
    uint8_t UNKNOWN_0to3[4];      // 0,1,2,3
    mCtrlSchedSub_a5_t sched[2];  // 4,5,6,7,8,9, 10,11,12,13,14,15
} PACK8 mCtrlSched_a5_t;

typedef struct {
    uint8_t circuit[4];  // 0..3 corresponding to the buttons on the remote
} PACK8 mCtrlLayout_a5_t;

typedef struct {
    uint8_t addressHi;   // 0
    uint8_t addressLo;   // 1
    uint8_t valueHi;     // 2
    uint8_t valueLo;     // 3
} PACK8 mPumpRegulateSet_a5_t;

typedef struct {
    uint8_t valueHi;     // 0
    uint8_t valueLo;     // 1
} PACK8 mPumpRegulateSetResp_a5_t;

typedef struct {
    uint8_t control;     // 0
} PACK8 mPumpControl_a5_t;

typedef struct {
    uint8_t mode;        // 0
} PACK8 mPumpMode_a5_t;

typedef struct {
    uint8_t state;       // 0
} PACK8 mPumpState_a5_t;

typedef struct {
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

typedef struct {
    uint8_t UNKNOWN_0;
} PACK8 mPump0xFF_a5_t;


/*
* IC messages, use to communicate with IntelliChlor
*/

typedef struct {
    uint8_t dst;  // destination
    uint8_t typ;  // message type
} PACK8 mHdr_ic_t;

typedef struct {
    uint8_t UNKNOWN_0;
} PACK8 mChlorPingReq_ic_t;

typedef struct {
    uint8_t UNKNOWN_0;
    uint8_t UNKNOWN_1;
} PACK8 mChlorPing_ic_t;

typedef struct {
    uint8_t UNKNOWN_0;
    char name[16];
} PACK8 mChlorName_ic_t;

typedef struct {
    uint8_t pct;
} PACK8 mChlorLvlSet_ic_t;

typedef struct {
    uint8_t salt;
    uint8_t err;  // call ; cold water
} PACK8 mChlorLvlSetResp_ic_t;

typedef struct {
    uint8_t UNKNOWN_0;
} PACK8 mChlor0X14_ic_t;

typedef enum {
    RS485_DIR_rx = 0,
    RS485_DIR_tx = 1
} rs485dir_t;


/*
 * for setting the heat
 */

typedef struct {
  struct {
    uint8_t setPoint;
    uint8_t heatSrc;
  } pool, spa;
} heatState_t;

/*
 */
typedef struct {
	protocol_t  proto;
	mHdr_a5_t   hdr;
	uint8_t     data[CONFIG_POOL_MSGDATA_BUFSIZE];
    uint16_t    chk;
} pentairMsg_t;
