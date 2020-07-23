#pragma once

#include <esp_system.h>
#include <cJSON.h>

#include "../network/network.h"

size_t state_to_json(poolstate_t * state, char * buf, size_t buf_len);

void cPool_AddActiveCircuitsToObject(cJSON * const root, char const * const key, uint16_t const active);
void cPool_AddSystemToObject(cJSON * const obj, char const * const key, uint const hour, uint const minute, float const fw);
void cPool_AddThermostatToObject(cJSON * const obj, char const * const key, uint8_t const temp, char const * const heat_src_str, uint8_t const heating);
void cPool_AddPumpRunningToObject(cJSON * const obj, char const * const key, uint8_t const pump_state);