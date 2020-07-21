#pragma once
#include "../proto/pentair.h"

void name_resetIdx(void);
char const * date_str(uint8_t const year, uint8_t const month, uint8_t const day);
char const * time_str(uint8_t const hours, uint8_t const minutes);
char const * name_mtCtrl(MT_CTRL_a5_t const mt, bool * const found);
char const * name_mtPump(MT_PUMP_a5_t const mt, bool const request, bool * const found);
char const * name_mtChlor(MT_CHLOR_ic_t const mt, bool * const found);
char const * name_hex8(uint8_t const value);
char const * name_hext16(uint16_t const value);
char const * name_name(char const * const name, uint_least8_t const len);
char const * name_chlor_state(uint8_t const chlorstate);
char const * name_pump_mode(uint16_t const value);
char const * name_pump_prg(uint16_t const address);
char const * name_circuit(uint const circuit);
uint name_circuit_nr(char const * const name);
char const * name_heat_src(uint8_t const value);
uint name_heat_src_nr(char const * const name);