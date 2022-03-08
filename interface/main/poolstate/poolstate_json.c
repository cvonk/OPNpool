/**
 * @brief Pool state: support, state to cJSON
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020-2022, Coert Vonk
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

static void
_addTimeToObject(cJSON * const obj, char const * const key, poolstate_time_t const * const time)
{
    cJSON_AddStringToObject(obj, key, network_time_str(time->hour, time->minute));
}

static void
_addDateToObject(cJSON * const obj, char const * const key, poolstate_date_t const * const date)
{
    cJSON_AddStringToObject(obj, key, network_date_str(date->year, date->month, date->day));
}

void
cJSON_AddTodToObject(cJSON * const obj, char const * const key, poolstate_tod_t const * const tod)
{
    cJSON * const item = _create_item(obj, key);
    _addTimeToObject(item, "time", &tod->time);
    _addDateToObject(item, "date", &tod->date);
}

void
cJSON_AddVersionToObject(cJSON * const obj, char const * const key, poolstate_version_t const * const version)
{
    cJSON_AddStringToObject(obj, key, network_version_str(version->major, version->minor));
}

static void
_addSystemToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON * const item = _create_item(obj, key);
    cJSON_AddTodToObject(item, "tod", &state->system.tod);
    cJSON_AddVersionToObject(item, "firmware", &state->system.version);
}

void
cJSON_AddSystemToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    _addSystemToObject(obj, key, state);
}

/**
 * poolstate->thermos
 **/

static void
_addThermostatToObject(cJSON * const obj, char const * const key, poolstate_thermo_t const * const thermostat,
                       bool const showTemp, bool showSp, bool const showSrc, bool const showHeating)
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
}

static void
_addThermosToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON_AddThermosToObject(obj, key, state->thermos, true, true, true, true);
}

void
cJSON_AddThermosToObject(cJSON * const obj, char const * const key, poolstate_thermo_t const * thermos,
                         bool const showTemp, bool showSp, bool const showSrc, bool const showHeating)
{
    cJSON * const item = _create_item(obj, key);
    for (uint ii = 0; ii < POOLSTATE_THERMO_TYP_COUNT; ii++, thermos++) {
        _addThermostatToObject(item, poolstate_thermo_str(ii), thermos,
                                    showTemp, showSp, showSrc, showHeating);
    }
}


/**
 * poolstate->scheds
 **/

static void
_addScheduleToObject(cJSON * const obj, char const * const key, poolstate_sched_t const * const sched, bool const showSched)
{
    cJSON * const item = _create_item(obj, key);
    if (showSched) {
        cJSON_AddStringToObject(item, "start", network_time_str(sched->start / 60, sched->start % 60));
        cJSON_AddStringToObject(item, "stop", network_time_str(sched->stop / 60, sched->stop % 60));
    }
}

static void
_addSchedsToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON_AddSchedsToObject(obj, key, state->scheds, true);
}

void
cJSON_AddSchedsToObject(cJSON * const obj, char const * const key, poolstate_sched_t const * scheds, bool const showSched)
{
    cJSON * const item = _create_item(obj, key);
    for (uint ii = 0; ii < POOLSTATE_SCHED_TYP_COUNT; ii++, scheds++) {
        _addScheduleToObject(item, network_circuit_str(scheds->circuit), scheds, showSched);
    }
}

/**
 * poolstate->temps
 **/

static void
_addTempToObject(cJSON * const obj, char const * const key, poolstate_temp_t const * const temp)
{
    if (temp->temp != 0xFF && temp->temp != 0x00) {
        cJSON_AddNumberToObject(obj, key, temp->temp);
    }
}

static void
_addTempsToObject(cJSON * const obj, char const * const key, poolstate_t const * state)
{
    cJSON * const item = _create_item(obj, key);
    poolstate_temp_t const * temp = state->temps;
    for (uint ii = 0; ii < POOLSTATE_TEMP_TYP_COUNT; ii++, temp++) {
        _addTempToObject(item, poolstate_temp_str(ii), temp);
    }
}

/**
 * poolstate->modes
 **/

static void
_addModesToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    poolstate_modes_t const * const modes = &state->modes;
    cJSON * const item = _create_item(obj, key);

    bool const * set = modes->set;
    for (uint ii = 0; ii < NETWORK_MODE_COUNT; ii++, set++) {
        cJSON_AddBoolToObject(item, network_mode_str(ii), *set);
    }
}

/**
 * poolstate->circuits, poolstate->delay 
 **/

static void
_addCircuitDetailToObject(cJSON * const obj, char const * const key, bool const * active)
{
    cJSON * const item = _create_item(obj, key);
    for (uint ii = 0; ii < NETWORK_CIRCUIT_COUNT; ii++, active++) {
        cJSON_AddBoolToObject(item, network_circuit_str(ii), *active);
    }
}

void
_addCircuitsToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    poolstate_circuits_t const * const circuits = &state->circuits;
    cJSON * const item = _create_item(obj, key);
    _addCircuitDetailToObject(item, "active", circuits->active);
    _addCircuitDetailToObject(item, "delay", circuits->delay);
}

/**
 * poolstate
 **/

void
cJSON_AddStateToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    cJSON * const item = _create_item(obj, key);
    _addSystemToObject(item, "system", state);
    _addTempsToObject(item, "temps", state);
    cJSON_AddThermosToObject(item, "thermos", state->thermos, true, false, true, true);
    cJSON_AddSchedsToObject(item, "scheds", state->scheds, true);
    _addModesToObject(item, "modes", state);
    _addCircuitsToObject(item, "circuits", state);
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

static void
_addPumpStateToObject(cJSON * const obj, char const * const key, uint8_t const state)
{
    cJSON_AddStringToObject(obj, key, network_pump_state_str(state));
}

void
cJSON_AddPumpRunningToObject(cJSON * const obj, char const * const key, bool const running)
{
    cJSON_AddBoolToObject(obj, key, running);
}

#if 0
void
cJSON_AddPumpStatusToObject(cJSON * const obj, char const * const key, poolstate_pump_t const * const pump)
{
    cJSON * const item = _create_item(obj, key);
    cJSON_AddPumpRunningToObject(item, "running", pump->running);
    cJSON_AddPumpModeToObject(item, "mode", pump->mode);
    _addPumpStateToObject(item, "state", pump->state);
}
#endif

static void
_addPumpToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    poolstate_pump_t const * const pump = &state->pump;
    cJSON * const item = _create_item(obj, key);
    _addTimeToObject(item, "time", &pump->time);
    cJSON_AddPumpModeToObject(item, "mode", pump->mode);
    cJSON_AddPumpRunningToObject(item, "running", pump->running);
    _addPumpStateToObject(item, "state", pump->state);
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

void
cJSON_AddPumpToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    _addPumpToObject(obj, key, state);
}

/**
 * poolstate->chlor
 **/

void
cJSON_AddChlorRespToObject(cJSON * const obj, char const * const key, poolstate_chlor_t const * const chlor)
{
    cJSON * const item = _create_item(obj, key);
    cJSON_AddNumberToObject(item, "salt", chlor->salt);
    cJSON_AddStringToObject(item, "status", poolstate_chlor_status_str(chlor->status));
}

static void
_addChlorToObject(cJSON * const obj, char const * const key, poolstate_t const * const state)
{
    poolstate_chlor_t const * const chlor = &state->chlor;
    cJSON * const item = _create_item(obj, key);
    cJSON_AddStringToObject(item, "name", chlor->name);
    cJSON_AddNumberToObject(item, "pct", chlor->pct);
    cJSON_AddNumberToObject(item, "salt", chlor->salt);
    cJSON_AddStringToObject(item, "status", poolstate_chlor_status_str(chlor->status));
}

/**
 * and finally .. poolstate itself
 **/

typedef void (* poolstate_json_fnc_t)(cJSON * const obj, char const * const key, poolstate_t const * const state);

typedef struct poolstate_json_dispatch_t {
    poolstate_elem_typ_t const  typ;
    char const * const          name;
    poolstate_json_fnc_t const  fnc;
} poolstate_json_dispatch_t;

static poolstate_json_dispatch_t _dispatches[] = {
    { POOLSTATE_ELEM_TYP_SYSTEM,   "system",   _addSystemToObject   },
    { POOLSTATE_ELEM_TYP_TEMP,     "temps",    _addTempsToObject    },
    { POOLSTATE_ELEM_TYP_THERMO,   "thermos",  _addThermosToObject  },
    { POOLSTATE_ELEM_TYP_SCHED,    "scheds",   _addSchedsToObject   },
    { POOLSTATE_ELEM_TYP_CIRCUITS, "circuits", _addCircuitsToObject },
    { POOLSTATE_ELEM_TYP_MODES,    "modes",    _addModesToObject    },
    { POOLSTATE_ELEM_TYP_PUMP,     "pump",     _addPumpToObject     },
    { POOLSTATE_ELEM_TYP_CHLOR,    "chlor",    _addChlorToObject    },
};

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
