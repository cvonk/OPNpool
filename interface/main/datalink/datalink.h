#pragma once
#include <sdkconfig.h>
#include <esp_system.h>

#include "../skb/skb.h"
#include "../rs485/rs485.h"

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

typedef uint8_t datalink_preamble_a5_t[3];
typedef uint8_t datalink_preamble_ic_t[2];

typedef struct datalink_hdr_ic_t {
    uint8_t dst;  // destination
    uint8_t typ;  // message type
} PACK8 datalink_hdr_ic_t;

typedef struct datalink_hdr_a5_t {
    uint8_t ver;  // protocol version id
    uint8_t dst;  // destination
    uint8_t src;  // source
    uint8_t typ;  // message type
    uint8_t len;  // # of data bytes following
} PACK8 datalink_hdr_a5_t;

typedef union datalink_hdr_t {
    datalink_hdr_ic_t ic;
    datalink_hdr_a5_t a5;
} PACK8 datalink_hdr_t;

typedef struct datalink_head_a5_t {
    uint8_t                ff;
    datalink_preamble_a5_t preamble;
    datalink_hdr_a5_t      hdr;
} PACK8 datalink_head_a5_t;

typedef struct datalink_head_ic_t {
    uint8_t                ff;
    datalink_preamble_ic_t preamble;
    datalink_hdr_ic_t      hdr;
} PACK8 datalink_head_ic_t;

typedef union datalink_head_t {
    datalink_head_ic_t ic;
    datalink_head_a5_t a5;
} datalink_head_t;
#define DATALINK_MAX_HEAD_SIZE (sizeof(datalink_head_t))

/**
 * datalink_data_t
 **/

#define DATALINK_MAX_DATA_SIZE (NETWORK_DATA_MAX_SIZE)

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
#define DATALINK_MAX_TAIL_SIZE (sizeof(datalink_tail_t))

/* datalink.c */
datalink_addrgroup_t datalink_groupaddr(uint16_t const addr);
uint8_t datalink_devaddr(uint8_t group, uint8_t const id);
uint16_t datalink_calc_crc(uint8_t const * const start, uint8_t const * const stop);
extern datalink_preamble_a5_t datalink_preamble_a5;
extern datalink_preamble_ic_t datalink_preamble_ic;

/* datalink_rx.c */
bool datalink_rx_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt);

/* datalink_tx.c */
void datalink_tx_queue_pkt(rs485_handle_t const rs485_handle, datalink_pkt_t * const pkt);

/* datalink_str.c */
char const * datalink_prot_str(datalink_prot_t const prot);
