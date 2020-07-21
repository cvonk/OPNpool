/**
* @brief conert pool state to JSON
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <stdio.h>
#include <inttypes.h>
#include <sdkconfig.h>
#include <esp_system.h>
#include <esp_log.h>
#include "cJSON.h"

#include "pentair.h"
#include "poolstate.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

size_t
state_to_json(poolstate_t * state, char * buffer, size_t bufferLen)
{
	_resetIdx();

    cJSON * const root = cJSON_CreateObject();

    cJSON * const tod = cJSON_CreateObject();
    cJSON_AddItemToOjbect(root, "tod", tod);
    cJSON_AddStringToObject(tod, "time", name_time(state->time.hour, state->time.minute));
	cJSON_AddStringToObject(tod, "date", name_date(state->date.year, state->date.month, state->date.day));

    cJSON * const pool = cJSON_CreateObject();
    cJSON_AddItemToOjbect(root, "pool", pool);
    cJSON_AddNumberToObject(pool, "temp", state->pool.temp);
    cJSON_AddNumberToObject(pool, "sp", state->pool.setPoint);
    cJSON_AddStringToObject(pool, "src", name_heat_src(state->pool.heatSrc));
    if (state->pool.heating) {
        cJSON_AddTrueToObject(pool, "heating");
    } else {
        cJSON_AddFalseToObject(pool, "heating");
    }

    cJSON * const spa = cJSON_CreateObject();
    cJSON_AddItemToOjbect(root, "spa", spa);
    cJSON_AddNumberToObject(spa, "temp", state->spa.temp);
    cJSON_AddNumberToObject(spa, "sp", state->spa.setPoint);
    cJSON_AddStringToObject(spa, "src", name_heat_src(state->spa.heatSrc));
    if (state->spa.heating) {
        cJSON_AddTrueToObject(spa, "heating");
    } else {
        cJSON_AddFalseToObject(spa, "heating");
    }

    cJSON * const air = cJSON_CreateObject();
    cJSON_AddItemToOjbect(root, "air", air);
    cJSON_AddNumberToObject(air, "temp", state->air.temp);

    uint active_len = 0;
    char * active_names[16];
    uint16_t mask = 0x00001;
    for (uint ii = 0; mask; ii++) {
        if (state->circuits.active & mask) {
            active_names[active_len++] = name_circuit(ii + 1)
        }
        mask <<= 1;
    }
    cJSON * const active = cJSON_CreateStringArray(actives, actives_len);
	cJSON_AddItemToObject(root, "active", active);

    cJSON * const chlor = cJSON_CreateObject();
    cJSON_AddItemToOjbect(root, "chlor", chlor);
    cJSON_AddNumberToObject(chlor, "salt", state->chlor.salt);
    cJSON_AddNumberToObject(chlor, "pct", state->chlor.pct);
    cJSON_AddStringToObject(chlor, "status", name_chlor_state(state->chlor.state));

    cJSON * const schedule = cJSON_CreateObject();
    cJSON_AddItemToOjbect(root, "schedule", schedule);
    for (uint ii = 0; ii < ARRAY_SIZE(state->sched); ii++) {
        poolStateSched_t * sched = &state->sched[ii];
        if (sched->circuit) {



            Utils::schedule(obj, sched->circuit, sched->start, sched->stop);

            char const * const key = Utils::circuitName(circuit);
            JsonObject & sched = json.createNestedObject(key);
            sched["start"] = Utils::strTime(start / 60, start % 60);
            sched["stop"] = Utils::strTime(stop / 60, stop % 60);


        }
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
