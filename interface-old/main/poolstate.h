#pragma once

#include <inttypes.h>
#include <sdkconfig.h>
#include "pooltypes.h"
#include "rs485.h"

class PoolState {
  public:
    void begin(void);
    void update(sysState_t * sys);
    //bool receive(HardwareSerial * rs485);
    size_t getStateAsJson(char * buffer, size_t bufferLen);
    heatState_t getHeatState(void);
    bool getCircuitState(char const * const key);
    uint8_t getSetPoint(char const * const key);
    char const * getHeatSrc(char const * const key);
  private:
    sysState_t * state_;
    struct {
        uint_least8_t  hex;
        uint_least16_t json;
    } memUsage_ = {};
};
