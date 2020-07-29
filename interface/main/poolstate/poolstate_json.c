/**
 * @brief Pool state: support, state to cJSON
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

#include "../utils/utils.h"
#include "../network/network.h"
#include "poolstate.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

/**
 * poolstate->system
 **/

void
cJSON_AddTimeToObject(cJSON * const obj, char const * const key, poolstate_time_t const * const time)
{
    cJSON_AddStringToObject(obj, key, network_time_str(time->hour, time->minute));
}

void
cJSON_AddDateToObject(cJSON * const obj, char const * const key, poolstate_date_t const * const date)
{
    cJSON_AddStringToObject(obj, key, network_date_str(date->year, date->month, date->day));
}

void
cJSON_AddTodToObject(cJSON * const obj, char const * const key, poolstate_tod_t const * const tod)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);
    cJSON_AddTimeToObject(item, "time", &tod->time);
    cJSON_AddDateToObject(item, "date", &tod->date);
}

void
cJSON_AddVersionToObject(cJSON * const obj, char const * const key, poolstate_version_t const * const version)
{
    cJSON_AddStringToObject(obj, key, network_version_str(version->major, version->minor));
}

void
cJSON_AddSystemToObject(cJSON * const obj, char const * const key, poolstate_system_t const * const system)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);
    cJSON_AddTodToObject(item, "tod", &system->tod);
    cJSON_AddVersionToObject(item, "firmware", &system->version);
}

/**
 * poolstate->thermostats
 **/

void
cJSON_AddSchedToObject(cJSON * const obj, char const * const key, poolstate_sched_t const * const sched)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);
    cJSON_AddStringToObject(item, "start", network_time_str(sched->start / 60, sched->start % 60));
    cJSON_AddStringToObject(item, "stop", network_time_str(sched->stop / 60, sched->stop % 60));
}

void
cJSON_AddThermostatToObject(cJSON * const obj, char const * const key, poolstate_thermostat_t const * const thermostat,
                            bool const showTemp, bool showSp, bool const showSrc, bool const showHeating, bool const showSched)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);
    if (showTemp) {
        cJSON_AddNumberToObject(item, "temp", thermostat->temp);
    }
    if (showSp) {
        cJSON_AddNumberToObject(item, "sp", thermostat->set_point);
    }
    if (showSrc) {
        cJSON_AddStringToObject(item, "src", network_heat_src_str(thermostat->heat_src));
    }
    if (showHeating) {
        cJSON_AddBoolToObject(item, "heating", thermostat->heating);
    }
    if (showSched) {
        cJSON_AddSchedToObject(item, "sched", &thermostat->sched);
    }
}

void
cJSON_AddThermostatsToObject(cJSON * const obj, char const * const key, poolstate_thermostat_t const * thermostats,
                             bool const showTemp, bool showSp, bool const showSrc, bool const showHeating, bool const showSched)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);

    for (uint ii = 0; ii < POOLSTATE_THERMOSTAT_COUNT; ii++, thermostats++) {
        cJSON_AddThermostatToObject(item, poolstate_thermostat_str(ii), thermostats,
                                    showTemp, showSp, showSrc, showHeating, showSched);
    }
}

/**
 * poolstate->temps
 **/

void
cJSON_AddTempToObject(cJSON * const obj, char const * const key, poolstate_temp_t const * const temp)
{
    if (temp->temp != 0xFF) {
        cJSON_AddNumberToObject(obj, key, temp->temp);
    }
}

void
cJSON_AddTempsToObject(cJSON * const obj, char const * const key, poolstate_temp_t const * temps)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);

    for (uint ii = 0; ii < POOLSTATE_TEMP_COUNT; ii++, temps++) {
        cJSON_AddTempToObject(item, poolstate_temp_str(ii), temps);
    }
}

/**
 * poolstate->circuits
 **/

void
cJSON_AddActiveCircuitsToObject(cJSON * const obj, char const * const key, bool const * active)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);

    for (uint ii = 0; ii < NETWORK_CIRCUIT_COUNT; ii++, active++) {
        cJSON_AddBoolToObject(item, network_circuit_str(ii), *active);
    }
}

void
cJSON_AddCircuitsToObject(cJSON * const obj, char const * const key, poolstate_circuits_t const * const circuits)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);

    cJSON_AddActiveCircuitsToObject(item, "active", circuits->active);
    cJSON_AddNumberToObject(item, "delay", circuits->delay);
}

/**
 * poolstate->
 **/

void
cJSON_AddStateToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON_AddSystemToObject(obj, "system", &state->system);
    cJSON_AddTempsToObject(obj, "temps", state->temps);
    cJSON_AddThermostatsToObject(obj, "thermostats", state->thermostats, true, false, true, true, false);
    cJSON_AddActiveCircuitsToObject(obj, "active", state->circuits.active);
}


/**
 * poolstate->pump
 **/

void
cJSON_AddPumpPrgToObject(cJSON * const obj, char const * const key, uint16_t const value)
{
    cJSON_AddNumberToObject(obj, key, value);
}

void
cJSON_AddPumpCtrlToObject(cJSON * const obj, char const * const key, uint8_t const ctrl)
{
    char const * str;
    switch (ctrl) {
        case 0x00: str = "local"; break;
        case 0xFF: str = "remote"; break;
        default: str = hex8_str(ctrl);
    }
    cJSON_AddStringToObject(obj, key, str);
}

void
cJSON_AddPumpModeToObject(cJSON * const obj, char const * const key, uint8_t const mode)
{
    cJSON_AddStringToObject(obj, key, network_pump_mode_str(mode));
}

void
cJSON_AddPumpRunningToObject(cJSON * const obj, char const * const key, bool const running)
{
    cJSON_AddBoolToObject(obj, key, running);
}

void
cJSON_AddPumpStatusToObject(cJSON * const obj, char const * const key, poolstate_pump_t const * const pump)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);
    cJSON_AddPumpRunningToObject(item, "running", pump->running);
    cJSON_AddPumpModeToObject(item, "mode", pump->mode);
    cJSON_AddNumberToObject(item, "status", pump->status);
}

void
cJSON_AddPumpToObject(cJSON * const obj, char const * const key, poolstate_pump_t const * const pump)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);
    cJSON_AddTimeToObject(item, "time", &pump->time);
    cJSON_AddPumpModeToObject(item, "mode", pump->mode);
    cJSON_AddPumpRunningToObject(item, "running", pump->running);
    cJSON_AddNumberToObject(item, "status", pump->status);
    cJSON_AddNumberToObject(item, "pwr", pump->pwr);
    cJSON_AddNumberToObject(item, "rpm", pump->rpm);
    if (pump->gpm) {
        cJSON_AddNumberToObject(item, "gpm", pump->gpm);
    }
    if (pump->pct) {
        cJSON_AddNumberToObject(item, "pct", pump->pct);
    }
    cJSON_AddNumberToObject(item, "err", pump->mode);
    cJSON_AddNumberToObject(item, "timer", pump->mode);
}

/**
 * poolstate->chlor
 **/

void
cJSON_AddChlorRespToObject(cJSON * const obj, char const * const key, poolstate_chlor_t const * const chlor)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);
    cJSON_AddNumberToObject(item, "salt", chlor->salt);
    cJSON_AddStringToObject(item, "status", poolstate_chlor_state_str(chlor->status));
}

void
cJSON_AddChlorToObject(cJSON * const obj, char const * const key, poolstate_chlor_t const * const chlor)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);
    cJSON_AddStringToObject(item, "name", chlor->name);
    cJSON_AddNumberToObject(item, "pct", chlor->pct);
    cJSON_AddNumberToObject(item, "salt", chlor->salt);
    cJSON_AddStringToObject(item, "status", poolstate_chlor_state_str(chlor->status));
}

/**
 * and finally .. poolstate itself
 **/

static void
_poolstate_to_object(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, "state", item);
    cJSON_AddSystemToObject(item, "system", &state->system);
    cJSON_AddTempsToObject(item, "temps", state->temps);
    cJSON_AddThermostatsToObject(item, "thermostats", state->thermostats, true, true, true, true, true);
    cJSON_AddCircuitsToObject(item, "circuits", &state->circuits);
    cJSON_AddPumpToObject(item, "pump", &state->pump);
    cJSON_AddChlorToObject(item, "chlor", &state->chlor);
}

size_t
poolstate_to_json(poolstate_t const * const state, char * const buf, size_t const buf_len)
{
    name_reset_idx();

    cJSON * const obj = cJSON_CreateObject();
    _poolstate_to_object(obj, "state", state);

    assert( cJSON_PrintPreallocated(obj, buf, buf_len, false) );
	cJSON_Delete(obj);

	return strlen(buf);
}
