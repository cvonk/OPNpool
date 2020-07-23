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



uint8_t network_ic_len(uint8_t const ic_typ);
