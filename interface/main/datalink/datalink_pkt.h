#pragma once
#include <esp_system.h>

#include "../skb/skb.h"

#define ALIGN( type ) __attribute__((aligned( __alignof__( type ) )))
#define PACK( type )  __attribute__((aligned( __alignof__( type ) ), packed ))
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
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

typedef uint8_t datalink_data_t;

/**
 * datalink_pkt_t
 **/

// state info retained between successive datalink_rx() calls
typedef struct datalink_pkt_t {
	  datalink_prot_t    prot;
    uint8_t            prot_typ;  // message type
    uint8_t            src;  // source
    uint8_t            dst;  // destination
    datalink_data_t *  data;
    size_t             data_len;
    skb_handle_t       skb;
} datalink_pkt_t;

