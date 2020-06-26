/**
 * Queue for messages to be transmitted over the Pentair RS-485 bus
 */

#pragma once

#include "cbuffer.h"
#include "rs485.h"
#include "pooltypes.h"
#include "poolstate.h"

typedef uint_least8_t bufBufIdx_t;
uint_least8_t const kBufElementCount = 4;

typedef union {
    mCtrlCircuitSet_a5_t circuitSet;
    mCtrlHeatSet_a5_t heatSet;
    uint8_t raw[0];
} PACK8 elementData_t;

typedef struct {
    MT_CTRL_a5_t typ;
    uint_least8_t dataLen;
    elementData_t data;
} PACK8 element_t;

class TransmitQueue {
  public:
    TransmitQueue(PoolState & poolState) : poolState_(poolState) {};
    void begin();
    void transmit(Rs485 * rs485);
    void enqueue( char const * const name, char const * const val );
  private:
    CBUF<bufBufIdx_t, kBufElementCount, element_t> txBuffer_;
    PoolState & poolState_;
};