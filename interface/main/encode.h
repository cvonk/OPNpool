/**
 * Encodes Messages to be transmitted over the Pentair RS-485 bus
 */

#pragma once
#include <inttypes.h>
#include "pooltypes.h"
#include "poolstate.h"
#include "transmitqueue.h"

class EncodeA5 {
  public:
    static void circuitMsg(element_t * element, 
                           uint8_t const circuit,
                           uint8_t const value);
    static void heatMsg(element_t * element,  
	                      uint8_t const poolTempSetpoint,
	                      uint8_t const spaTempSetpoint,
				                uint8_t const heatSrc);
};
