#pragma once

#include <esp_system.h>
#include <cJSON.h>

#include "../network/network.h"

size_t state_to_json(poolstate_t * state, char * buf, size_t buf_len);

void cPool_AddPumpPrgToObject(cJSON * const obj, char const * const key, uint16_t const value);
void cPool_AddPumpCtrlToObject(cJSON * const obj, char const * const key, uint8_t const ctrl);
void cPool_AddPumpModeToObject(cJSON * const obj, char const * const key, uint8_t const mode);
void cPool_AddPumpRunningToObject(cJSON * const obj, char const * const key, bool const pump_state_running);
void cPool_AddPumpStatusToObject(cJSON * const obj, char const * const key, poolstate_pump_t const * const pump_state);
void cPool_AddFirmwareToObject(cJSON * const obj, char const * const key, poolstate_version_t const * const version);
void cPool_AddDateToObject(cJSON * const obj, char const * const key, poolstate_date_t const * const date);
void cPool_AddTimeToObject(cJSON * const obj, char const * const key, poolstate_time_t const * const time);
void cPool_AddTodToObject(cJSON * const obj, char const * const key, poolstate_tod_t const * const state_tod);
void cPool_AddActiveCircuitsToObject(cJSON * const obj, char const * const key, uint16_t const active);
void cPool_AddThermostatToObject(cJSON * const obj, char const * const key, poolstate_thermostat_t const * const thermostat);
void cPool_AddTempToObject(cJSON * const obj, char const * const key, uint8_t const temp);
void cPool_AddSchedToObject(cJSON * const obj, char const * const key, uint16_t const start, uint16_t const stop);
void cPool_AddStateToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);
