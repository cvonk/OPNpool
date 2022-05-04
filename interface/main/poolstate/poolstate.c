/**
 * @brief OPNpool - maintains the state information for the pool
 *
 * © Copyright 2014, 2019, 2022, Coert Vonk
 * 
 * This file is part of OPNpool.
 * OPNpool is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * OPNpool is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with OPNpool. 
 * If not, see <https://www.gnu.org/licenses/>.
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: Copyright 2014,2019,2022 Coert Vonk
 */

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
	_protected.state->chlor.name[0] = '\0';
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

esp_err_t
poolstate_get(poolstate_t * const state)
{
    bool valid;
    xSemaphoreTake( _protected.xMutex, portMAX_DELAY );
    {
        valid = _protected.valid;
        memcpy(state, _protected.state, sizeof(poolstate_t));
    }
    xSemaphoreGive( _protected.xMutex );
    return valid ? ESP_OK : ESP_FAIL;
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
	current.pool.setPoint = _protected.state->thermos[POOLSTATE_THERMO_TYP_pool].setPoint;
	current.pool.heatSrc = _protected.state->thermos[POOLSTATE_THERMO_TYP_pool].heatSrc;
	current.spa.setPoint = _protected.state->thermos[POOLSTATE_THERMO_TYP_spa].setPoint;
	current.spa.heatSrc = _protected.state->thermos[POOLSTATE_THERMO_TYP_spa].heatSrc;
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
		return _protected.state->thermos[POOLSTATE_THERMO_TYP_pool].setPoint;
	}
	if (strcmp(key, "spa") == 0) {
		return _protected.state->thermos[POOLSTATE_THERMO_TYP_spa].setPoint;
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

