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

static char const * const TAG = "poolstate_get";

static void
_alloc_str(char * * const value, char const * const str)
{
    assert( asprintf(value, "%s", str) >= 0);
}

static void
_alloc_uint(char * * const value, uint16_t const num)
{
    assert( asprintf(value, "%u", num) >= 0);
}

static void
_alloc_bool(char * * const value, bool const num)
{
    assert( asprintf(value, "%s", num ? "true" : "false") >= 0);
}
static esp_err_t
_system(poolstate_t const * const state, uint8_t const typ, uint8_t const idx, poolstate_get_value_t * value)
{
    poolstate_system_t const * const system = &state->system;
    switch (typ) {
        case POOLSTATE_ELEM_SYSTEM_TYP_VERSION:
            _alloc_str(value, network_version_str(system->version.major, system->version.minor));
            break;
        case POOLSTATE_ELEM_SYSTEM_TYP_TIME:
            _alloc_str(value, network_time_str(system->tod.time.hour, system->tod.time.minute));
            break;
        default:
            ESP_LOGW(TAG, "%s unknown sub_typ(%u)", __func__, typ);
            return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t
_temp(poolstate_t const * const state, uint8_t const typ, uint8_t const idx, poolstate_get_value_t * const value)
{
    poolstate_temp_t const * const temp = &state->temps[idx];
    switch (typ) {
        case POOLSTATE_ELEM_TEMP_TYP_TEMP:
            _alloc_uint(value, temp->temp);
            break;
        default:
            ESP_LOGW(TAG, "%s unknown sub_typ(%u)", __func__, typ);
            return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t
_thermostat(poolstate_t const * const state, uint8_t const typ, uint8_t const idx, poolstate_get_value_t * const value)
{
    poolstate_thermo_t const * const thermostat = &state->thermos[idx];
    switch (typ) {
        case POOLSTATE_ELEM_THERMO_TYP_TEMP:
            _alloc_uint(value, thermostat->temp);
            break;
        case POOLSTATE_ELEM_THERMO_TYP_SET_POINT:
            _alloc_uint(value, thermostat->set_point);
            break;
        case POOLSTATE_ELEM_THERMO_TYP_HEAT_SRC:
            _alloc_str(value, network_heat_src_str(thermostat->heat_src));
            break;
        case POOLSTATE_ELEM_THERMO_TYP_HEATING:
            _alloc_bool(value, thermostat->heating);
            break;
        case POOLSTATE_ELEM_THERMO_TYP_START:
            _alloc_str(value, network_time_str(thermostat->sched.start / 60, thermostat->sched.start % 60));
            break;
        case POOLSTATE_ELEM_THERMO_TYP_STOP:
            _alloc_str(value, network_time_str(thermostat->sched.stop / 60, thermostat->sched.stop % 60));
            break;
        default:
            ESP_LOGW(TAG, "%s unknown sub_typ(%u)", __func__, typ);
            return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t
_pump(poolstate_t const * const state, uint8_t const typ, uint8_t const idx, poolstate_get_value_t * const value)
{
    poolstate_pump_t const * const pump = &state->pump;
    switch (typ) {
        case POOLSTATE_ELEM_PUMP_TYP_MODE:
            _alloc_str(value, network_pump_mode_str(pump->mode));
            break;
        case POOLSTATE_ELEM_PUMP_TYP_RUNNING:
            _alloc_bool(value, pump->running);
            break;
        case POOLSTATE_ELEM_PUMP_TYP_STATUS:
            _alloc_uint(value, pump->status);
            break;
        case POOLSTATE_ELEM_PUMP_TYP_PWR:
            _alloc_uint(value, pump->pwr);
            break;
        case POOLSTATE_ELEM_PUMP_TYP_GPM:
            _alloc_uint(value, pump->gpm);
            break;
        case POOLSTATE_ELEM_PUMP_TYP_RPM:
            _alloc_uint(value, pump->rpm);
            break;
        case POOLSTATE_ELEM_PUMP_TYP_PCT:
            _alloc_uint(value, pump->pct);
            break;
        case POOLSTATE_ELEM_PUMP_TYP_ERR:
            _alloc_uint(value, pump->err);
            break;
        case POOLSTATE_ELEM_PUMP_TYP_TIMER:
            _alloc_uint(value, pump->timer);
            break;
        default:
            ESP_LOGW(TAG, "%s unknown sub_typ(%u)", __func__, typ);
            return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * chlor
 **/

static esp_err_t
_chlor(poolstate_t const * const state, uint8_t const typ, uint8_t const idx, poolstate_get_value_t * const value)
{
    poolstate_chlor_t const * const chlor = &state->chlor;
    switch (typ) {
        case POOLSTATE_ELEM_CHLOR_TYP_NAME:
            _alloc_str(value, chlor->name);
            break;
        case POOLSTATE_ELEM_CHLOR_TYP_PCT:
            _alloc_uint(value, chlor->pct);
            break;
        case POOLSTATE_ELEM_CHLOR_TYP_SALT:
            _alloc_uint(value, chlor->salt);
            break;
        case POOLSTATE_ELEM_CHLOR_TYP_STATUS:
            _alloc_str(value, poolstate_chlor_status_str(chlor->status));
            break;
        default:
            ESP_LOGW(TAG, "%s unknown sub_typ(%u)", __func__, typ);
            return ESP_FAIL;
    }
    return ESP_OK;
}


/**
 * all together now
 **/

typedef esp_err_t (* dispatch_fnc_t)(poolstate_t const * const state, uint8_t const sub_typ, uint8_t const idx, poolstate_get_value_t * const value);

typedef struct dispatch_t {
    uint8_t const  typ;
    dispatch_fnc_t fnc;
} dispatch_t;

static dispatch_t const _dispatches[] = {
    { POOLSTATE_ELEM_TYP_SYSTEM, _system},
    { POOLSTATE_ELEM_TYP_TEMP,   _temp},
    { POOLSTATE_ELEM_TYP_THERMO, _thermostat},
    { POOLSTATE_ELEM_TYP_PUMP,   _pump},
    { POOLSTATE_ELEM_TYP_CHLOR,  _chlor},
};

esp_err_t
poolstate_get_value(poolstate_t const * const state, poolstate_get_params_t const * const params, poolstate_get_value_t * const value)
{
    dispatch_t const * dispatch = _dispatches;
    for (uint8_t ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
        if (params->elem_typ == dispatch->typ) {

            return dispatch->fnc(state, params->elem_sub_typ, params->idx, value);  // caller MUST free *value
        }
    }
    return ESP_FAIL;
}
