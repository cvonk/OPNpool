#pragma once
#include <sdkconfig.h>
#include <esp_system.h>

#include "../rs485/rs485.h"
#include "../network/network_hdr.h"

// struct/emum mapping
#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

typedef struct datalink_hdr_t {
    uint8_t pro;  // protocol version id
    uint8_t dst;  // destination
    uint8_t src;  // source
    uint8_t typ;  // message type
    uint8_t len;  // # of data bytes following
} PACK8 datalink_hdr_t;

typedef struct datalink_pkt_t {
	NETWORK_PROT_t proto;
	datalink_hdr_t hdr;
	uint8_t        data[CONFIG_POOL_DATALINK_LEN];
    uint16_t       chk;
} datalink_pkt_t;

// #define DATALINK_A5_CTRL_MSGTYP_SET (0x80)
// #define DATALINK_A5_CTRL_MSGTYP_REQ (0xC0)

// use macro "magic" to get an enum and matching name_* function (in name.c)
#define DATALINK_A5_CTRL_MSGTYP_MAP(XX) \
  XX(0x01, SET_ACK)     \
  XX(0x86, CIRCUIT_SET) \
  XX(0x02, STATE)       \
  XX(0x82, STATE_SET)   \
  XX(0xC2, STATE_REQ )  \
  XX(0x05, TIME)        \
  XX(0x85, TIME_SET)    \
  XX(0xC5, TIME_REQ)    \
  XX(0x08, HEAT)        \
  XX(0x88, HEAT_SET)    \
  XX(0xC8, HEAT_REQ)    \
  XX(0x1E, SCHED)       \
  XX(0x9E, SCHED_SET)   \
  XX(0xDE, SCHED_REQ)   \
  XX(0x21, LAYOUT)      \
  XX(0xA1, LAYOUT_SET)  \
  XX(0xE1, LAYOUT_REQ)

typedef enum {
#define XX(num, name) DATALINK_A5_CTRL_MSGTYP_##name = num,
  DATALINK_A5_CTRL_MSGTYP_MAP(XX)
#undef XX
} DATALINK_A5_CTRL_MSGTYP_t;

// use macro "magic" to get an enum and matching name_* function (in name.c)
// FYI occasionally there is a src=0x10 dst=0x60 typ=0xFF with data=[0x80]; pump doesn't reply to it
#define DATALINK_A5_PUMP_MSGTYP_MAP(XX) \
  XX(0x01, REGULATE) \
  XX(0x04, CTRL) \
  XX(0x05, MODE) \
  XX(0x06, STATE) \
  XX(0x07, STATUS) \
  XX(0xff, 0xFF)

typedef enum {
#define XX(num, name) DATALINK_A5_PUMP_MSGTYP_##name = num,
  DATALINK_A5_PUMP_MSGTYP_MAP(XX)
#undef XX
} DATALINK_A5_PUMP_MSGTYP_t;

// use macro "magic" to get an enum and matching name_* function (in name.c)
// FYI tere is a 0x14, // has dst=0x50 data=[0x00]
 #define DATALINK_IC_CHLOR_MSGTYP_MAP(XX) \
  XX(0x00, PING_REQ) \
  XX(0x01, PING) \
  XX(0x03, NAME) \
  XX(0x11, LEVEL_SET) \
  XX(0x12, LEVEL_RESP) \
  XX(0x14, X14)

typedef enum {
#define XX(num, name) DATALINK_IC_CHLOR_MSGTYP_##name = num,
  DATALINK_IC_CHLOR_MSGTYP_MAP(XX)
#undef XX
} DATALINK_IC_CHLOR_MSGTYP_t;

bool datalink_receive_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt);
