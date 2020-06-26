/**
 * Decodes Messages received over the Pentair RS-485 bus, and returns them in JSON and as a binary state.
 */

#pragma once
#include "ArduinoJson.h"
#include "pooltypes.h"

/**
 * Decode Messages received over the Pentair RS-485 bus, and returns them in JSON and as a binary state.
 */
class Decode {
  public:
    static void decode(pentairMsg_t * msg, sysState_t * sys, JsonObject * root);
};