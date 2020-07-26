#pragma once
#include <sdkconfig.h>
#include <esp_system.h>

#include "../skb/skb.h"
#include "../rs485/rs485.h"
#include "../network/network_msgs.h"

#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif
#if !(defined CONFIG_POOL_DBG_DATALINK)
#  define CONFIG_POOL_DBG_DATALINK (0)
#endif
#if !(defined CONFIG_POOL_DBG_DATALINK_ONERROR)
#  define CONFIG_POOL_DBG_DATALINK_ONERROR (0)
#endif

/* order MUST match _proto_infos[] */
#define DATALINK_PROT_MAP(XX) \
  XX(0x00, IC)      \
  XX(0x01, A5_CTRL) \
  XX(0x02, A5_PUMP)

typedef enum {
#define XX(num, name) DATALINK_PROT_##name = num,
  DATALINK_PROT_MAP(XX)
#undef XX
} datalink_prot_t;

#define DATALINK_ADDRGROUP_MAP(XX) \
  XX(0x00, ALL)  \
  XX(0x01, CTRL) \
  XX(0x02, REMOTE) \
  XX(0x05, CHLOR) \
  XX(0x06, PUMP) \
  XX(0x09, X09)

typedef enum {
#define XX(num, name) DATALINK_ADDRGROUP_##name = num,
  DATALINK_ADDRGROUP_MAP(XX)
#undef XX
} datalink_addrgroup_t;

/**
 * datalink_head_t
 **/

typedef uint8_t datalink_a5_preamble_t[3];
typedef uint8_t datalink_ic_preamble_t[2];

typedef union datalink_preamble_t {
    datalink_ic_preamble_t ic;
    datalink_a5_preamble_t a5;
} PACK8 datalink_preamble_t;

typedef struct datalink_ic_hdr_t {
    uint8_t dst;  // destination
    uint8_t typ;  // message type
} PACK8 datalink_ic_hdr_t;

typedef struct datalink_a5_hdr_t {
    uint8_t ver;  // protocol version id
    uint8_t dst;  // destination
    uint8_t src;  // source
    uint8_t typ;  // message type
    uint8_t len;  // # of data bytes following
} PACK8 datalink_a5_hdr_t;

typedef union datalink_hdr_t {
    datalink_ic_hdr_t ic;
    datalink_a5_hdr_t a5;
} PACK8 datalink_hdr_t;

typedef struct datalink_head_a5_t {
    uint8_t              ff;
    datalink_a5_preamble_t preamble;
    datalink_a5_hdr_t    hdr;
} PACK8 datalink_head_a5_t;

typedef struct datalink_head_ic_t {
    uint8_t                ff;
    datalink_ic_preamble_t preamble;
    datalink_ic_hdr_t      hdr;
} PACK8 datalink_head_ic_t;

typedef union datalink_head_t {
    datalink_head_ic_t ic;
    datalink_head_a5_t a5;
} datalink_head_t;

#define DATALINK_IC_HEAD_SIZE (sizeof(datalink_head_ic_t))
#define DATALINK_A5_HEAD_SIZE (sizeof(datalink_head_a5_t))
#define DATALINK_MAX_HEAD_SIZE (sizeof(datalink_head_t))

/**
 * datalink_data_t
 **/

typedef uint8_t datalink_data_t;

#define DATALINK_MAX_DATA_SIZE (sizeof(network_msg_data_t))

/**
 * datalink_tail_t
 **/

typedef struct datalink_tail_a5_t {
    uint8_t  crc[2];
} PACK8 datalink_tail_a5_t;

typedef struct datalink_tail_ic_t {
    uint8_t  crc[1];
} PACK8 datalink_tail_ic_t;

typedef union datalink_tail_t {
    datalink_tail_ic_t ic;
    datalink_tail_a5_t a5;
} datalink_tail_t;

#define DATALINK_IC_TAIL_SIZE (sizeof(datalink_tail_ic_t))
#define DATALINK_A5_TAIL_SIZE (sizeof(datalink_tail_a5_t))
#define DATALINK_MAX_TAIL_SIZE (sizeof(datalink_tail_t))

/**
 * datalink_pkt_t
 **/

typedef struct datalink_hdt_ic_t {
    datalink_head_ic_t *  head;
    datalink_data_t *     data;
    datalink_tail_ic_t *  tail;
} datalink_hdt_ic_t;

typedef struct datalink_pkt_t {
	datalink_prot_t    prot;
    datalink_head_t *  head;
    datalink_data_t *  data;
    datalink_tail_t *  tail;
    size_t             data_len;
    skb_handle_t       skb;
} datalink_pkt_t;

/**
 * DATALINK_CTRL_TYP_t
 **/

// #define DATALINK_CTRL_TYP_SET (0x80)
// #define DATALINK_CTRL_TYP_REQ (0xC0)

#define DATALINK_CTRL_TYP_MAP(XX) \
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
#define XX(num, name) DATALINK_CTRL_TYP_##name = num,
  DATALINK_CTRL_TYP_MAP(XX)
#undef XX
} datalink_ctrl_typ_t;

/**
 * DATALINK_PUMP_TYP_t
 **/

// FYI occasionally there is a src=0x10 dst=0x60 typ=0xFF with data=[0x80]; pump doesn't reply to it
#define DATALINK_PUMP_TYP_MAP(XX) \
  XX(0x01, REGULATE) \
  XX(0x04, CTRL) \
  XX(0x05, MODE) \
  XX(0x06, STATE) \
  XX(0x07, STATUS) \
  XX(0xff, 0xFF)

typedef enum {
#define XX(num, name) DATALINK_PUMP_TYP_##name = num,
  DATALINK_PUMP_TYP_MAP(XX)
#undef XX
} datalink_pump_typ_t;

/**
 * DATALINK_CHLOR_TYP_t
 **/

// FYI tere is a 0x14, // has dst=0x50 data=[0x00]
 #define DATALINK_CHLOR_TYP_MAP(XX) \
  XX(0x00, PING_REQ)   \
  XX(0x01, PING_RESP)  \
  XX(0x03, NAME)       \
  XX(0x11, LEVEL_SET)  \
  XX(0x12, LEVEL_RESP) \
  XX(0x14, X14)

typedef enum {
#define XX(num, name) DATALINK_CHLOR_TYP_##name = num,
  DATALINK_CHLOR_TYP_MAP(XX)
#undef XX
} datalink_chlor_typ_t;


/* datalink.c */
extern datalink_a5_preamble_t datalink_preamble_a5;
extern datalink_ic_preamble_t datalink_preamble_ic;
datalink_addrgroup_t datalink_groupaddr(uint16_t const addr);
uint8_t datalink_devaddr(uint8_t group, uint8_t const id);
uint16_t datalink_calc_crc(uint8_t const * const start, uint8_t const * const stop);

/* datalink_rx.c */
bool datalink_rx_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt);

/* datalink_tx.c */
void datalink_tx_pkt(rs485_handle_t const rs485_handle, skb_handle_t const txb, datalink_prot_t const prot, datalink_ctrl_typ_t const typ);

/* datalink_str.c */
char const * datalink_prot_str(datalink_prot_t const prot);
char const * datalink_pump_typ_str(datalink_pump_typ_t typ);
char const * datalink_ctrl_type_str(datalink_ctrl_typ_t typ);
char const * datalink_chlor_typ_str(datalink_chlor_typ_t typ);
