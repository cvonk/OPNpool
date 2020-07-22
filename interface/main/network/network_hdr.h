#pragma once
#include <sdkconfig.h>
#include <esp_system.h>

// struct/emum mapping
#ifndef PACK8
#define PACK8  __attribute__((aligned( __alignof__( uint8_t ) ), packed ))
#endif

typedef enum {
    NETWORK_PROT_A5 = 0,
    NETWORK_PROT_IC,
} NETWORK_PROT_t;

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

uint8_t network_ic_len(uint8_t const ic_typ);
