/**
 * Receive messages from the Pentair RS-485 bus
 */

#pragma once
#include <inttypes.h>
#include "ArduinoJson.h"
#include "pooltypes.h"
#include "poolstate.h"

class Receiver {
  public:
    static void begin(void) {};
    static bool receive(Rs485 * rs485, sysState_t * sys);   // return true when there is a transmit opportunity (a complete message has been received)
};
