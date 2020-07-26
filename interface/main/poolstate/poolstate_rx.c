/**
 * @brief Pool state: updates the state in response to network messages
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
#include <cJSON.h>

#include "../utils/utils.h"
#include "../network/network.h"
#include "../ipc/ipc.h"
#include "poolstate.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "poolstate_rx";

/**
 * ctrl
 **/

static void
_ctrl_time(cJSON * const dbg, network_msg_ctrl_time_t const * const msg, poolstate_t * const state)
{
    state->system.tod.time.minute = msg->minute;
    state->system.tod.time.hour = msg->hour;
    state->system.tod.date.day = msg->day;
    state->system.tod.date.month = msg->month;
    state->system.tod.date.year = msg->year;

    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddTodToObject(dbg, "tod", &state->system.tod);
    }
}

static void
_ctrl_heat(cJSON * const dbg, network_msg_ctrl_heat_t const * const msg, poolstate_t * const state)
{
    state->thermostats[POOLSTATE_THERMOSTAT_POOL].temp = msg->poolTemp;
    state->thermostats[POOLSTATE_THERMOSTAT_POOL].set_point = msg->poolTempSetpoint;
    state->thermostats[POOLSTATE_THERMOSTAT_POOL].heat_src = msg->heatSrc & 0x03;
    state->thermostats[POOLSTATE_THERMOSTAT_SPA].temp = msg->spaTemp;
    state->thermostats[POOLSTATE_THERMOSTAT_SPA].set_point = msg->spaTempSetpoint;
    state->thermostats[POOLSTATE_THERMOSTAT_SPA].heat_src = msg->heatSrc >> 2;

    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddThermostatsToObject(dbg, "thermostats", state->thermostats, true, true, true, false, false);
    }
}

static void
_ctrl_heat_set(cJSON * const dbg, network_msg_ctrl_heat_set_t const * const msg, poolstate_t * const state)
{
    state->thermostats[POOLSTATE_THERMOSTAT_POOL].set_point = msg->poolTempSetpoint;
    state->thermostats[POOLSTATE_THERMOSTAT_POOL].heat_src = msg->heatSrc & 0x03;
    state->thermostats[POOLSTATE_THERMOSTAT_SPA].set_point = msg->spaTempSetpoint;
    state->thermostats[POOLSTATE_THERMOSTAT_SPA].heat_src = msg->heatSrc >> 2;

    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddThermostatsToObject(dbg, "thermostats", state->thermostats, false, true, true, false, false);
    }
}

static void
_ctrl_circuit_set(cJSON * const dbg, network_msg_ctrl_circuit_set_t const * const msg)
{
    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddNumberToObject(dbg, network_circuit_str(msg->circuit - 1), msg->value);
    }
}

static void
_ctrl_sched(cJSON * const dbg, network_msg_ctrl_sched_t const * const msg, poolstate_t * const state)
{
    mCtrlSchedSub_a5_t const * msg_sched = msg->scheds;
    poolstate_thermostat_t * state_thermostat = state->thermostats;

    for (uint  ii = 0; ii < POOLSTATE_THERMOSTAT_COUNT; ii++, msg_sched++, state_thermostat++) {
        state_thermostat->sched.circuit = msg_sched->circuit;
        state_thermostat->sched.start = (uint16_t)msg_sched->prgStartHi << 8 | msg_sched->prgStartLo;
        state_thermostat->sched.stop = (uint16_t)msg_sched->prgStopHi << 8 | msg_sched->prgStopLo;
    }
    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddThermostatsToObject(dbg, "thermostats", state->thermostats, false, false, false, false, true);
    }
}

static void
_ctrl_state(cJSON * const dbg, network_msg_ctrl_state_t const * const msg, poolstate_t * state)
{
    state->system.tod.time.minute = msg->minute;
    state->system.tod.time.hour = msg->hour;
    state->system.version.major = msg->major;
    state->system.version.minor = msg->minor;
    state->temps[POOLSTATE_TEMP_AIR].temp = msg->airTemp;
    state->temps[POOLSTATE_TEMP_SOLAR].temp = msg->solarTemp;
    state->thermostats[POOLSTATE_THERMOSTAT_POOL].temp = msg->poolTemp;
    state->thermostats[POOLSTATE_THERMOSTAT_POOL].heat_src = msg->heatSrc & 0x03;
    state->thermostats[POOLSTATE_THERMOSTAT_POOL].heating = msg->heating & 0x04;
    state->thermostats[POOLSTATE_THERMOSTAT_SPA].temp = msg->poolTemp;
    state->thermostats[POOLSTATE_THERMOSTAT_SPA].heat_src = msg->heatSrc >> 2;
    state->thermostats[POOLSTATE_THERMOSTAT_SPA].heating = msg->heating & 0x08;

    bool * state_active = state->circuits.active;
    uint16_t msg_mask = 0x00001;
    uint16_t const msg_active = ((uint16_t)msg->activeHi << 8) | msg->activeLo;
    for (uint ii = 0; ii < NETWORK_CIRCUIT_COUNT; ii++, state_active++) {
        *state_active = msg_active & msg_mask;
        msg_mask <<= 1;
    }
    state->circuits.delay = msg->delay;

    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddStateToObject(dbg, "state", state);
    }
}

/**
 * pump
 **/

static void
_pump_reg_set(cJSON * const dbg, network_msg_pump_reg_set_t const * const msg)
{
    uint const address = (msg->addressHi << 8) | msg->addressLo;
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddPumpPrgToObject(dbg, network_pump_prg_str(address), value);
    }
}

static void
_pump_reg_set_resp(cJSON * const dbg, network_msg_pump_reg_resp_t const * const msg)
{
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddPumpPrgToObject(dbg, "resp", value);
    }
}


static void
_pump_ctrl(cJSON * const dbg, network_msg_pump_ctrl_t const * const msg)
{
    if (CONFIG_POOL_DBG_POOLSTATE) {
       cJSON_AddPumpCtrlToObject(dbg, "ctrl", msg->ctrl);
    }
}

static void
_pump_mode(cJSON * const dbg, network_msg_pump_mode_t const * const msg, poolstate_t * const state)
{
    state->pump.mode = msg->mode;

    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddPumpModeToObject(dbg, "mode", msg->mode);
    }
}

static void
_pump_run(cJSON * const dbg, network_msg_pump_run_t const * const msg, poolstate_t * const state)
{
    bool const running = msg->running == 0x0A;
    bool const not_running = msg->running == 0x04;
    if (!running && !not_running) {
        return;
    }
    state->pump.running = running;
    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddPumpRunningToObject(dbg, "running", state->pump.running);
    }
}

static void
_pump_state(cJSON * const dbg, network_msg_pump_state_t const * const msg, poolstate_t * const state)
{
    bool const running = msg->running == 0x0A;
    bool const not_running = msg->running == 0x04;
    if (!running && !not_running) {
        return;
    }
    state->pump.running = running;
    state->pump.mode = msg->mode;
    state->pump.status = msg->status;
    state->pump.pwr = ((uint16_t)msg->powerHi << 8) | msg->powerLo;
    state->pump.rpm = ((uint16_t)msg->rpmHi << 8) | msg->rpmLo;
    state->pump.gpm = msg->gpm;
    state->pump.pct = msg->pct;
    state->pump.err = msg->err;
    state->pump.timer = msg->timer;
    state->pump.time.hour = msg->hour;
    state->pump.time.minute = msg->minute;

    if (CONFIG_POOL_DBG_POOLSTATE) {
       cJSON_AddPumpToObject(dbg, "status", &state->pump);
    }
}

static void
_ctrl_set_ack(cJSON * const dbg, network_msg_ctrl_set_ack_t const * const msg)
{
    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddNumberToObject(dbg, "ack", msg->typ);
    }
}

/**
 * chlor
 **/

static void
_chlor_name(cJSON * const dbg, network_msg_chlor_name_t const * const msg, poolstate_t * const state)
{
    size_t name_size = sizeof(state->chlor.name);
    strncpy(state->chlor.name, msg->name, name_size);
    state->chlor.name[name_size - 1] = '\0';

    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddStringToObject(dbg, "name", state->chlor.name);
    }
}

static void
_chlor_set(cJSON * const dbg, network_msg_chlor_level_set_t const * const msg, poolstate_t * const state)
{
    state->chlor.pct = msg->pct;

    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddNumberToObject(dbg, "pct", state->chlor.pct);
    }
}

static void
_chlor_set_resp(cJSON * const dbg, network_msg_chlor_level_resp_t const * const msg, poolstate_t * const state)
{
    state->chlor.salt = (uint16_t)msg->salt * 50;
    bool const lowFlow = (msg->err & 0x01);
    if (state->chlor.salt < 2600) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_VERYLOW_SALT;
    } else if (state->chlor.salt < 2800) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_LOW_SALT;
    } else if (state->chlor.salt > 4500) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_HIGH_SALT;
    } else if (lowFlow) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_LOW_FLOW;
    } else {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_OK;
    }
    if (CONFIG_POOL_DBG_POOLSTATE) {
        cJSON_AddChlorRespToObject(dbg, "chlor", &state->chlor);
    }
}

bool
poolstate_rx_update(network_msg_t const * const msg, poolstate_t * const state, ipc_t * const ipc_for_dbg)
{
	name_reset_idx();

    poolstate_t old_state;
    (void)poolstate_get(&old_state);
    memcpy(state, &old_state, sizeof(poolstate_t));

    cJSON * const dbg = cJSON_CreateObject();
    switch(msg->typ) {
        case NETWORK_MSG_TYP_CTRL_SET_ACK:  // response to various set requests
            _ctrl_set_ack(dbg, msg->u.ctrl_set_ack);
            break;
        case NETWORK_MSG_TYP_CTRL_CIRCUIT_SET:  // set circuit request (there appears to be no separate "get circuit request")
            _ctrl_circuit_set(dbg, msg->u.ctrl_circuit_set);
            break;
        case NETWORK_MSG_TYP_CTRL_SCHED_REQ:
            break;
        case NETWORK_MSG_TYP_CTRL_SCHED:  // schedule: get response / set request
            _ctrl_sched(dbg, msg->u.ctrl_sched, state);
            break;
        case NETWORK_MSG_TYP_CTRL_STATE_REQ:
            break;
        case NETWORK_MSG_TYP_CTRL_STATE:  // state: get response / set request
            _ctrl_state(dbg, msg->u.ctrl_state, state);
            break;
        case NETWORK_MSG_TYP_CTRL_STATE_SET:
            break;
        case NETWORK_MSG_TYP_CTRL_TIME_REQ:
            break;
        case NETWORK_MSG_TYP_CTRL_TIME:
        case NETWORK_MSG_TYP_CTRL_TIME_SET:
            _ctrl_time(dbg, msg->u.ctrl_time, state);
            break;
        case NETWORK_MSG_TYP_CTRL_HEAT_REQ:
            break;
        case NETWORK_MSG_TYP_CTRL_HEAT:
            _ctrl_heat(dbg, msg->u.ctrl_heat, state);
            break;
        case NETWORK_MSG_TYP_CTRL_HEAT_SET:
            _ctrl_heat_set(dbg, msg->u.ctrl_heat_set, state);
            break;
        case NETWORK_MSG_TYP_CTRL_LAYOUT_REQ:
        case NETWORK_MSG_TYP_CTRL_LAYOUT:
        case NETWORK_MSG_TYP_CTRL_LAYOUT_SET:
            break;
        case NETWORK_MSG_TYP_PUMP_REG_SET:
            _pump_reg_set(dbg, msg->u.pump_reg_set);
            break;
        case NETWORK_MSG_TYP_PUMP_REG_RESP:
            _pump_reg_set_resp(dbg, msg->u.pump_reg_set_resp);
            break;
        case NETWORK_MSG_TYP_PUMP_CTRL_SET:
        case NETWORK_MSG_TYP_PUMP_CTRL_RESP:
            _pump_ctrl(dbg, msg->u.pump_ctrl);
            break;
        case NETWORK_MSG_TYP_PUMP_MODE_SET:
        case NETWORK_MSG_TYP_PUMP_MODE_RESP:
            _pump_mode(dbg, msg->u.pump_mode, state);
            break;
        case NETWORK_MSG_TYP_PUMP_RUN_SET:
        case NETWORK_MSG_TYP_PUMP_RUN_RESP:
            _pump_run(dbg, msg->u.pump_run, state);
            break;
        case NETWORK_MSG_TYP_PUMP_STATE_REQ:
             break;
        case NETWORK_MSG_TYP_PUMP_STATE_RESP:
            _pump_state(dbg, msg->u.pump_state, state);
            break;
        case NETWORK_MSG_TYP_CHLOR_PING_REQ:
        case NETWORK_MSG_TYP_CHLOR_PING_RESP:
            break;
        case NETWORK_MSG_TYP_CHLOR_NAME:
            _chlor_name(dbg, msg->u.chlor_name, state);
            break;
        case NETWORK_MSG_TYP_CHLOR_LEVEL_SET:
            _chlor_set(dbg, msg->u.chlor_level_set, state);
            break;
        case NETWORK_MSG_TYP_CHLOR_LEVEL_RESP:
            _chlor_set_resp(dbg, msg->u.chlor_level_resp, state);
            break;
        case NETWORK_MSG_TYP_NONE:  // to please the gcc
            break;  //
    }
    if (CONFIG_POOL_DBG_POOLSTATE) {
        size_t const json_size = 1024;
        char * const json = malloc(json_size);
        assert( cJSON_PrintPreallocated(dbg, json, json_size, false) );
        ESP_LOGI(TAG, "{%s: %s}", network_msg_typ_str(msg->typ), json);
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_DBG, json, ipc_for_dbg);
        free(json);
    }
    cJSON_Delete(dbg);

    bool const state_changed = memcmp(state, &old_state, sizeof(poolstate_t)) != 0;
    if (state_changed) {
        poolstate_set(state);
    }
    return state_changed;
}
