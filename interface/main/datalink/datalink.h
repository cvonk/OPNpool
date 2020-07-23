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

bool datalink_receive_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt);
