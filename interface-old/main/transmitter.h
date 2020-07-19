/**
 * Transmit messages over the Pentair RS-485 bus
 */

#pragma once

#include <stdint.h>
#include "ArduinoJson.hpp"
#include "rs485.h"
#include "poolstate.h"
#include "transmitqueue.h"

class Transmitter {
  public:
    static void send_a5(Rs485 * rs485, JsonObject * root, element_t const * const element);
};