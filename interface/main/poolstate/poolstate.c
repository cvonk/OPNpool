/**
 * @brief Pool state: maintains the state information for the pool
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "../network/network.h"
#include "poolstate.h"

static struct poolstate_prot_t {
    SemaphoreHandle_t xMutex;
    poolstate_t * state;
    bool valid;
} _protected;

void
poolstate_init(void)
{
    _protected.xMutex = xSemaphoreCreateMutex();
    _protected.state = malloc(sizeof(poolstate_t));
    assert(_protected.xMutex && _protected.state);
    memset(_protected.state, 0, sizeof(poolstate_t));
}

void
poolstate_set(poolstate_t const * const state)
{
    xSemaphoreTake( _protected.xMutex, portMAX_DELAY );
    {
        _protected.valid = true;
        memcpy(_protected.state, state, sizeof(poolstate_t));
    }
    xSemaphoreGive( _protected.xMutex );
}

bool
poolstate_get(poolstate_t * const state)
{
    bool valid;
    xSemaphoreTake( _protected.xMutex, portMAX_DELAY );
    {
        valid = _protected.valid;
        memcpy(state, _protected.state, sizeof(poolstate_t));
    }
    xSemaphoreGive( _protected.xMutex );
    return valid;
}

#if 0
bool
poolstate_get_circuit(char const * const key)
{
	uint16_t mask = 0x00001;
	for (uint_least8_t ii = 0; mask; ii++) {
		if (strcmp(key, network_circuit_str(ii + 1)) == 0) {
			return value & mask;
		}
		mask <<= 1;
	}
	return false;
}

heatpoolstate_t
poolstate_get_heat(void)
{
	heatpoolstate_t current;
	current.pool.setPoint = _protected.state->thermostats[POOLSTATE_THERMOSTAT_POOL].setPoint;
	current.pool.heatSrc = _protected.state->thermostats[POOLSTATE_THERMOSTAT_POOL].heatSrc;
	current.spa.setPoint = _protected.state->thermostats[POOLSTATE_THERMOSTAT_SPA].setPoint;
	current.spa.heatSrc = _protected.state->thermostats[POOLSTATE_THERMOSTAT_SPA].heatSrc;
	return current;
}

char const *
poolstate_get_heat_src(char const * const key)
{
	if (strcmp(key, "pool") == 0) {
		return _heatSrcStr(this->poolstate_->pool.heatSrc);
	}
	if (strcmp(key, "spa") == 0) {
		return _heatSrcStr(this->poolstate_->spa.heatSrc);
	}
	return "err";
}

uint8_t
poolstate_get_heat_sp(char const * const key)
{
	if (strcmp(key, "pool") == 0) {
		return _protected.state->thermostats[POOLSTATE_THERMOSTAT_POOL].setPoint;
	}
	if (strcmp(key, "spa") == 0) {
		return _protected.state->thermostats[POOLSTATE_THERMOSTAT_SPA].setPoint;
	}
	return 0;
}

void
poolstate_name_schedule(JsonObject & json, uint8_t const circuit, uint16_t const start, uint16_t const stop)
{
	char const * const key = _circuitName(circuit);
	JsonObject & sched = json.createNestedObject(key);
	sched["start"] = Utils::strTime(start / 60, start % 60);
	sched["stop"] = Utils::strTime(stop / 60, stop % 60);
}

#endif

