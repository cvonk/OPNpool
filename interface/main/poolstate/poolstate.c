/**
 * @brief Pool state and access functions
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <stdio.h>
#include <string.h>
#include <sdkconfig.h>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "poolstate.h"

static struct poolstate_prot_t {
    SemaphoreHandle_t xMutex;
    poolstate_t * state;
} _protected;

void
poolstate_init(void)
{
    _protected.xMutex = xSemaphoreCreateMutex();
    _protected.state = malloc(sizeof(poolstate_t));
    assert(_protected.xMutex && _protected.state);
}

void
poolstate_set(poolstate_t const * const state)
{
    xSemaphoreTake( _protected.xMutex, portMAX_DELAY );
    {
        memcpy(_protected.state, state, sizeof(poolstate_t));
    }
    xSemaphoreGive( _protected.xMutex );
}

void
poolstate_get(poolstate_t * const state)
{
    xSemaphoreTake( _protected.xMutex, portMAX_DELAY );
    {
        memcpy(state, _protected.state, sizeof(poolstate_t));
    }
    xSemaphoreGive( _protected.xMutex );
}

#if 0
bool
poolstate_get_circuit(char const * const key)
{
	uint16_t mask = 0x00001;
	for (uint_least8_t ii = 0; mask; ii++) {
		if (strcmp(key, name_circuit(ii + 1)) == 0) {
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
	current.pool.setPoint = _protected.state->pool.setPoint;
	current.pool.heatSrc = _protected.state->pool.heatSrc;
	current.spa.setPoint = _protected.state->spa.setPoint;
	current.spa.heatSrc = _protected.state->spa.heatSrc;
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
		return _protected.state->pool.setPoint;
	}
	if (strcmp(key, "spa") == 0) {
		return _protected.state->spa.setPoint;
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

size_t
poolstate_getStateAsJson(char * buffer, size_t bufferLen)
{
	StaticJsonBuffer<CONFIG_POOL_JSON_BUFSIZE> jsonBuffer;
	JsonObject * json = &jsonBuffer.createObject();

	poolstate_t * sys = _protected.state;
	_resetIdx();

	if (json && sys) {
		{
			JsonObject & obj = json->createNestedObject("tod");
			obj["time"] = _timeStr(sys->time.hour, sys->time.minute);
			obj["date"] = _dateStr(sys->date.year, sys->date.month, sys->date.day);
		} {
			JsonObject & obj = json->createNestedObject("pool");
			obj["temp"] = sys->pool.temp;
			obj["sp"] = sys->pool.setPoint;
			obj["src"] = _heatSrcStr(sys->pool.heatSrc);
			obj["heating"] = sys->pool.heating;
		} {
			JsonObject & obj = json->createNestedObject("spa");
			obj["temp"] = sys->spa.temp;
			obj["sp"] = sys->spa.setPoint;
			obj["src"] = _heatSrcStr(sys->spa.heatSrc);
			obj["heating"] = sys->spa.heating;
		} {
			JsonObject & obj = json->createNestedObject("air");
			obj["temp"] = sys->air.temp;
		} {
			Utils::activeCircuits(*json, "active", sys->circuits.active);
#if 0
			jsonActive(*json, "delay", sys->circuits.delay);
#endif
		} {
			JsonObject & obj = json->createNestedObject("chlor");
			obj["salt"] = sys->chlor.salt;
			obj["pct"] = sys->chlor.pct;
			obj["status"] = Utils::chlorStateName(sys->chlor.state);
		} {
			JsonObject & obj = json->createNestedObject("schedule");
			for (uint_least8_t ii = 0; ii < ARRAY_SIZE(sys->sched); ii++) {
				poolStateSched_t * sched = &sys->sched[ii];
				if (sched->circuit) {
					Utils::schedule(obj, sched->circuit, sched->start, sched->stop);
				}
			}
		} {
			JsonObject & obj = json->createNestedObject("pump");
			obj["running"] = sys->pump.running;
			obj["mode"] = _pumpModeStr(sys->pump.mode);
			obj["status"] = sys->pump.status;
			obj["pwr"] = sys->pump.pwr;
			obj["rpm"] = sys->pump.rpm;
			obj["err"] = sys->pump.err;
		}
		return Utils::jsonPrint(json, buffer, bufferLen);
	}
	return 0;
}
#endif

