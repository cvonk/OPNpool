/**
* @brief conert pool state to JSON
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <cJSON.h>

#include "../presentation/presentation.h"
#include "../presentation/name.h"
#include "poolstate.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "poolstate";

size_t
state_to_json(poolstate_t * state, char * buf, size_t buf_len)
{
	name_reset_idx();

    cJSON * const root = cJSON_CreateObject();

    cJSON * const tod = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "tod", tod);
    cJSON_AddStringToObject(tod, "time", name_time(state->time.hour, state->time.minute));
	cJSON_AddStringToObject(tod, "date", name_date(state->date.year, state->date.month, state->date.day));

    cJSON * const pool = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "pool", pool);
    cJSON_AddNumberToObject(pool, "temp", state->pool.temp);
    cJSON_AddNumberToObject(pool, "sp", state->pool.setPoint);
    cJSON_AddStringToObject(pool, "src", name_heat_src(state->pool.heatSrc));
    if (state->pool.heating) {
        cJSON_AddTrueToObject(pool, "heating");
    } else {
        cJSON_AddFalseToObject(pool, "heating");
    }

    cJSON * const spa = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "spa", spa);
    cJSON_AddNumberToObject(spa, "temp", state->spa.temp);
    cJSON_AddNumberToObject(spa, "sp", state->spa.setPoint);
    cJSON_AddStringToObject(spa, "src", name_heat_src(state->spa.heatSrc));
    if (state->spa.heating) {
        cJSON_AddTrueToObject(spa, "heating");
    } else {
        cJSON_AddFalseToObject(spa, "heating");
    }

    cJSON * const air = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "air", air);
    cJSON_AddNumberToObject(air, "temp", state->air.temp);

    uint active_len = 0;
    char const * active_names[16];
    uint16_t mask = 0x00001;
    for (uint ii = 0; mask; ii++) {
        if (state->circuits.active & mask) {
            active_names[active_len++] = name_circuit(ii + 1);
        }
        mask <<= 1;
    }
    cJSON * const active = cJSON_CreateStringArray(active_names, active_len);
	cJSON_AddItemToObject(root, "active", active);

    cJSON * const chlor = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "chlor", chlor);
    cJSON_AddNumberToObject(chlor, "salt", state->chlor.salt);
    cJSON_AddNumberToObject(chlor, "pct", state->chlor.pct);
    cJSON_AddStringToObject(chlor, "status", name_chlor_state(state->chlor.state));

    cJSON * const schedule = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "schedule", schedule);
    for (uint ii = 0; ii < ARRAY_SIZE(state->sched); ii++) {
        poolstate_sched_t * sched = &state->sched[ii];
        if (sched->circuit) {

            cJSON * const ss = cJSON_CreateObject();
            cJSON_AddItemToObject(schedule, name_circuit(sched->circuit), ss);
            cJSON_AddStringToObject(ss, "start", name_time(sched->start / 60, sched->start % 60));
            cJSON_AddStringToObject(ss, "stop", name_time(sched->stop / 60, sched->stop % 60));
        }
    }

    cJSON * const pump = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "pump", pump);
    if (state->pump.running) {
        cJSON_AddTrueToObject(pump, "running");
    } else {
        cJSON_AddFalseToObject(pump, "running");
    }
    cJSON_AddStringToObject(pump, "mode", name_pump_mode(state->pump.mode));
    cJSON_AddNumberToObject(pump, "status", state->pump.status);
    cJSON_AddNumberToObject(pump, "pwr", state->pump.pwr);
    cJSON_AddNumberToObject(pump, "rpm", state->pump.rpm);
    cJSON_AddNumberToObject(pump, "err", state->pump.err);

    assert(cJSON_PrintPreallocated(root, buf, buf_len, false));
	cJSON_Delete(root);

	ESP_LOGI(TAG, "%s", buf);
	return strlen(buf);
}
