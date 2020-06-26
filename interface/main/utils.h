/**
 * Various support function for transmitting/receiving over the Pentair RS-485 bus
 */

#pragma once
#include "ArduinoJson.h"
#include "pooltypes.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

// also used to display date/time, add +50
#define K_STR_BUF_SIZE ((sizeof(mHdr_a5_t) + sizeof(mCtrlState_a5_t) + 1) * 3 + 50)

class Utils {
  public:
    static uint16_t calcCRC_a5(mHdr_a5_t const * const hdr, uint8_t const * const data);
    static uint8_t calcCRC_ic(mHdr_a5_t const * const hdr, uint8_t const * const data);

    static void resetIdx(void);
    static char const * strDate(uint8_t const year, uint8_t const month, uint8_t const day);
    static char const * strTime(uint8_t const hours, uint8_t const minutes);
    static char const * strHeatSrc(uint8_t const value);
    static char const * chlorStateName(uint8_t const chlorstate);
    static char const * strPumpMode(uint16_t const value);    
    static uint_least8_t circuitNr(char const * const name);
    static char const * strHex8(uint8_t const value);
    static char const * strHex16(uint16_t const value);
    static char const * mtCtrlName(MT_CTRL_a5_t const mt, bool * const found);
    static char const * mtPumpName(MT_PUMP_a5_t const mt, bool const request, bool * const found);
    static char const * mtChlorName(MT_CHLOR_ic_t const mt, bool * const found);
    static char const * strName(char const * const name, uint_least8_t const len);
    static char const * circuitName(uint8_t const circuit);
    static uint_least8_t heatSrcNr(char const * const name);
    static char const * strPumpPrgName(uint16_t const address);

    static void activeCircuits(JsonObject & json, char const * const key, uint16_t const value);
    static bool  circuitIsActive(char const * const key, uint16_t const value);
    static void schedule(JsonObject & json, uint8_t const circuit, uint16_t const start, uint16_t const stop);

    static addrGroup_t addrGroup(uint_least8_t const addr);
    static uint8_t addr(uint8_t group, uint8_t const id);

    static size_t jsonPrint(JsonObject * json, char * buffer, size_t bufferSize);    
    static void jsonRaw(pentairMsg_t * msg, JsonObject * json, char const * const key);
};
