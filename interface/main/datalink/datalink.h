#pragma once
#include <sdkconfig.h>
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

typedef struct mHdr_a5_t {
    uint8_t pro;  // protocol version
    uint8_t dst;  // destination
    uint8_t src;  // source
    uint8_t typ;  // message type
    uint8_t len;  // # of data bytes following
} PACK8 mHdr_a5_t;

typedef struct mHdr_ic_t {
    uint8_t dst;  // destination
    uint8_t typ;  // message type
} PACK8 mHdr_ic_t;

typedef struct datalink_msg_t {
	PROTOCOL_t  proto;
	mHdr_a5_t   hdr;
	uint8_t     data[CONFIG_POOL_DATALINK_LEN];
    uint16_t    chk;
} datalink_msg_t;

void datalink_task(void * ipc_void);

