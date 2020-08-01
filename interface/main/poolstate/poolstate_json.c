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

typedef void (* poolstate_json_fnc_t)(cJSON * const obj, char const * const key, poolstate_t const * const state);

typedef struct poolstate_json_dispatch_t {
    poolstate_elem_typ_t const  typ;
    char const * const          name;
    poolstate_json_fnc_t const  fnc;
} poolstate_json_dispatch_t;

poolstate_json_dispatch_t _dispatches[] = {
    { POOLSTATE_ELEM_TYP_SYSTEM,      "system",      cJSON_AddSystemToObject      },
    { POOLSTATE_ELEM_TYP_TEMPS,       "temps",       cJSON_AddTempsToObject       },
    { POOLSTATE_ELEM_TYP_THERMOSTATS, "thermostats", cJSON_AddThermostatsToObject },
    { POOLSTATE_ELEM_TYP_CIRCUITS,    "circuits",    cJSON_AddCircuitsToObject    },
    { POOLSTATE_ELEM_TYP_PUMP,        "pump",        cJSON_AddPumpToObject        },
    { POOLSTATE_ELEM_TYP_CHLOR,       "chlor",       cJSON_AddChlorToObject       },
};

static cJSON *
_create_item(cJSON * const obj, char const * const key)
{
    if (key == NULL) {
        return obj;
    }
    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, key, item);
    return item;
}

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
    cJSON * const item = _create_item(obj, key);
    cJSON_AddTimeToObject(item, "time", &tod->time);
    cJSON_AddDateToObject(item, "date", &tod->date);
}

void
cJSON_AddVersionToObject(cJSON * const obj, char const * const key, poolstate_version_t const * const version)
{
    cJSON_AddStringToObject(obj, key, network_version_str(version->major, version->minor));
}

void
cJSON_AddSystemToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON * const item = _create_item(obj, key);
    cJSON_AddTodToObject(item, "tod", &state->system.tod);
    cJSON_AddVersionToObject(item, "firmware", &state->system.version);
}

/**
 * poolstate->thermostats
 **/

void
cJSON_AddSchedToObject(cJSON * const obj, char const * const key, poolstate_sched_t const * const sched)
{
    cJSON * const item = _create_item(obj, key);
    cJSON_AddStringToObject(item, "start", network_time_str(sched->start / 60, sched->start % 60));
    cJSON_AddStringToObject(item, "stop", network_time_str(sched->stop / 60, sched->stop % 60));
}

void
cJSON_AddThermostatToObject(cJSON * const obj, char const * const key, poolstate_thermostat_t const * const thermostat,
                            bool const showTemp, bool showSp, bool const showSrc, bool const showHeating, bool const showSched)
{
    cJSON * const item = _create_item(obj, key);
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
cJSON_AddThermostatsToObject_generic(cJSON * const obj, char const * const key, poolstate_thermostat_t const * thermostats,
                                     bool const showTemp, bool showSp, bool const showSrc, bool const showHeating, bool const showSched)
{
    cJSON * const item = _create_item(obj, key);
    for (uint ii = 0; ii < POOLSTATE_THERMOSTAT_COUNT; ii++, thermostats++) {
        cJSON_AddThermostatToObject(item, poolstate_thermostat_str(ii), thermostats,
                                    showTemp, showSp, showSrc, showHeating, showSched);
    }
}

void
cJSON_AddThermostatsToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON * const item = _create_item(obj, key);
    cJSON_AddThermostatsToObject_generic(item, key, state->thermostats, true, true, true, true, true);
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
cJSON_AddTempsToObject(cJSON * const obj, char const * const key, poolstate_t const * state)
{
    cJSON * const item = _create_item(obj, key);
    poolstate_temp_t const * temp = state->temps;
    for (uint ii = 0; ii < POOLSTATE_TEMP_COUNT; ii++, temp++) {
        cJSON_AddTempToObject(item, poolstate_temp_str(ii), temp);
    }
}

/**
 * poolstate->circuits
 **/

void
cJSON_AddActiveCircuitsToObject(cJSON * const obj, char const * const key, bool const * active)
{
    cJSON * const item = _create_item(obj, key);
    for (uint ii = 0; ii < NETWORK_CIRCUIT_COUNT; ii++, active++) {
        cJSON_AddBoolToObject(item, network_circuit_str(ii), *active);
    }
}

void
cJSON_AddCircuitsToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    poolstate_circuits_t const * const circuits = &state->circuits;
    cJSON * const item = _create_item(obj, key);
    cJSON_AddActiveCircuitsToObject(item, "active", circuits->active);
    cJSON_AddNumberToObject(item, "delay", circuits->delay);
}

/**
 * poolstate->
 **/

void
cJSON_AddStateToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON * const item = _create_item(obj, key);
    cJSON_AddSystemToObject(item, "system", state);
    cJSON_AddTempsToObject(item, "temps", state);
    cJSON_AddThermostatsToObject_generic(item, "thermostats", state->thermostats, true, false, true, true, false);
    cJSON_AddActiveCircuitsToObject(item, "active", state->circuits.active);
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
    cJSON * const item = _create_item(obj, key);
    cJSON_AddPumpRunningToObject(item, "running", pump->running);
    cJSON_AddPumpModeToObject(item, "mode", pump->mode);
    cJSON_AddNumberToObject(item, "status", pump->status);
}

void
cJSON_AddPumpToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    poolstate_pump_t const * const pump = &state->pump;
    cJSON * const item = _create_item(obj, key);
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
    cJSON * const item = _create_item(obj, key);
    cJSON_AddNumberToObject(item, "salt", chlor->salt);
    cJSON_AddStringToObject(item, "status", poolstate_chlor_state_str(chlor->status));
}

void
cJSON_AddChlorToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    poolstate_chlor_t const * const chlor = &state->chlor;
    cJSON * const item = _create_item(obj, key);
    cJSON_AddStringToObject(item, "name", chlor->name);
    cJSON_AddNumberToObject(item, "pct", chlor->pct);
    cJSON_AddNumberToObject(item, "salt", chlor->salt);
    cJSON_AddStringToObject(item, "status", poolstate_chlor_state_str(chlor->status));
}

/**
 * and finally .. poolstate itself
 **/

char const *
poolstate_to_json(poolstate_t const * const state, poolstate_elem_typ_t const typ)
{
    name_reset_idx();
    cJSON * const obj = cJSON_CreateObject();
    poolstate_json_dispatch_t const * dispatch = _dispatches;
    for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
        bool const all_types = typ == POOLSTATE_ELEM_TYP_ALL;
        if (typ == dispatch->typ || all_types) {

            dispatch->fnc(obj, all_types ? dispatch->name : NULL, state);
        }
    }
    char const * const json = cJSON_PrintUnformatted(obj);
	cJSON_Delete(obj);
	return json;  // caller MUST free
}
