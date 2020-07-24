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

static void
_pump_reg_set(cJSON * const dbg, network_msg_pump_reg_set_t const * const msg)
{
    uint const address = (msg->addressHi << 8) | msg->addressLo;
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cPool_AddPumpPrgToObject(dbg, network_pump_prg_str(address), value);
    }
}

static void
_pump_reg_set_resp(cJSON * const dbg, network_msg_pump_reg_resp_t const * const msg)
{
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cPool_AddPumpPrgToObject(dbg, "now", value);
    }
}


static void
_pump_ctrl(cJSON * const dbg, network_msg_pump_ctrl_t const * const msg)
{
    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
       cPool_AddPumpCtrlToObject(dbg, "ctrl", msg->ctrl);
    }
}

static void
_pump_running(cJSON * const dbg, network_msg_pump_running_t const * const msg, poolstate_t * const state)
{
    bool const running = msg->running == 0x0A;
    bool const not_running = msg->running == 0x04;
    if (!running && !not_running) {
        return;
    }
    state->pump.running = running;

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cPool_AddPumpRunningToObject(dbg, "running", state->pump.running);
    }
}

static void
_pump_mode(cJSON * const dbg, network_msg_pump_mode_t const * const msg, poolstate_t * const state)
{
    state->pump.mode = msg->mode;

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cPool_AddPumpModeToObject(dbg, "mode", msg->mode);
    }
}

static void
_pump_status(cJSON * const dbg, network_msg_pump_status_t const * const msg, poolstate_t * const state)
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

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
       cPool_AddPumpStatusToObject(dbg, "status", state);
    }
}

static void
_ctrl_set_ack(cJSON * const dbg, network_msg_ctrl_set_ack_t const * const msg)
{
    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cJSON_AddNumberToObject(dbg, "ack", msg->typ);
    }
}

static void
_ctrl_circuit_set(cJSON * const dbg, network_msg_ctrl_circuit_set_t const * const msg)
{
    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cJSON_AddNumberToObject(dbg, network_circuit_str(msg->circuit), msg->value);
    }
}

static void
_ctrl_sched(cJSON * const dbg, network_msg_ctrl_sched_t const * const msg, poolstate_t * state)
{
    mCtrlSchedSub_a5_t const * msg_sched = msg->scheds;
    poolstate_sched_t * state_sched = state->scheds;

    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(dbg, "schedules", item);

    for (uint  ii = 0; ii < ARRAY_SIZE(state->scheds); ii++, msg_sched++, state_sched++) {

        state_sched->circuit = msg_sched->circuit;
        state_sched->start = (uint16_t)msg_sched->prgStartHi << 8 | msg_sched->prgStartLo;
        state_sched->stop = (uint16_t)msg_sched->prgStopHi << 8 | msg_sched->prgStopLo;

        if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
                cPool_AddSchedToObject(item, network_circuit_str(state_sched->circuit), state_sched->start, state_sched->stop);
        }
    }
}

static void
_ctrl_state(cJSON * const dbg, network_msg_ctrl_state_t const * const msg, poolstate_t * state)
{
    state->tod.time.minute = msg->minute;
    state->tod.time.hour = msg->hour;
    state->version.major = msg->major;
    state->version.minor = msg->minor;
    state->circuits.active = ((uint16_t)msg->activeHi << 8) | msg->activeLo;
    state->circuits.delay = msg->delayLo;
    state->pool.temp = msg->poolTemp;
    state->pool.heat_src = msg->heatSrc & 0x03;
    state->pool.heating = msg->heating & 0x04;
    state->spa.temp = msg->poolTemp;
    state->spa.heat_src = msg->heatSrc >> 2;
    state->spa.heating = msg->heating & 0x08;
    state->air.temp = msg->airTemp;
    state->solar.temp = msg->solarTemp;

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cPool_AddStateToObject(dbg, "state", state);
    }
}

static void
_ctrl_time(cJSON * const dbg, network_msg_ctrl_time_t const * const msg, poolstate_t * const state)
{
    state->tod.time.minute = msg->minute;
    state->tod.time.hour = msg->hour;
    state->tod.date.day = msg->day;
    state->tod.date.month = msg->month;
    state->tod.date.year = msg->year;

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cPool_AddTodToObject(dbg, "tod", &state->tod);
    }
}

static void
_ctrl_heat(cJSON * const dbg, network_msg_ctrl_heat_t const * const msg, poolstate_t * const state)
{
    state->pool.temp = msg->poolTemp;
    state->pool.set_point = msg->poolTempSetpoint;
    state->pool.heat_src = msg->heatSrc & 0x03;
    state->spa.temp = msg->spaTemp;
    state->spa.set_point = msg->spaTempSetpoint;
    state->spa.heat_src = msg->heatSrc >> 2;

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cPool_AddThermostatToObject(dbg, "pool", &state->pool);
        cPool_AddThermostatToObject(dbg, "spa", &state->spa);
    }
}

static void
_ctrl_heat_set(cJSON * const dbg, network_msg_ctrl_heat_set_t const * const msg, poolstate_t * const state)
{
    state->pool.set_point = msg->poolTempSetpoint;
    state->pool.heat_src = msg->heatSrc & 0x03;
    state->spa.set_point = msg->spaTempSetpoint;
    state->spa.heat_src = msg->heatSrc >> 2;

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cPool_AddThermostatSetToObject(dbg, "pool", &state->pool);
        cPool_AddThermostatSetToObject(dbg, "spa", &state->spa);
    }
}

static void
_chlor_name(cJSON * const dbg, network_msg_chlor_name_t const * const msg, poolstate_t * const state)
{
    strncpy(state->chlor.name, msg->name, sizeof(mChlorNameStr));

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cJSON_AddStringToObject(dbg, "name", state->chlor.name);
    }
}

static void
_chlor_set(cJSON * const dbg, network_msg_chlor_level_set_t const * const msg, poolstate_t * const state)
{
    state->chlor.pct = msg->pct;

    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
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
    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        cPool_AddChlorRespToObject(dbg, "chlor", state);
    }
}

bool
poolstate_rx_update(network_msg_t const * const msg, poolstate_t * const state, ipc_t * const ipc_for_dbg)
{
	name_reset_idx();

    poolstate_t old_state;
    poolstate_get(&old_state);
    memcpy(state, &old_state, sizeof(poolstate_t));

    cJSON * const dbg = cJSON_CreateObject();
    switch(msg->typ) {

        // msgs that cause a state update
        case NETWORK_MSG_TYP_PUMP_MODE:
            _pump_mode(dbg, msg->u.pump_mode, state);
            break;
        case NETWORK_MSG_TYP_PUMP_RUNNING:
            _pump_running(dbg, msg->u.pump_running, state);
            break;
        case NETWORK_MSG_TYP_PUMP_STATUS_REQ:
             break;
        case NETWORK_MSG_TYP_PUMP_STATUS:
            _pump_status(dbg, msg->u.pump_status, state);
            break;
        case NETWORK_MSG_TYP_CTRL_SCHED:  // schedule: get response / set request
            _ctrl_sched(dbg, msg->u.ctrl_sched, state);
            break;
        case NETWORK_MSG_TYP_CTRL_STATE:  // state: get response / set request
            _ctrl_state(dbg, msg->u.ctrl_state, state);
            break;
        case NETWORK_MSG_TYP_CTRL_TIME:
        case NETWORK_MSG_TYP_CTRL_TIME_SET:
            _ctrl_time(dbg, msg->u.ctrl_time, state);
            break;
        case NETWORK_MSG_TYP_CTRL_HEAT:
            _ctrl_heat(dbg, msg->u.ctrl_heat, state);
            break;
        case NETWORK_MSG_TYP_CTRL_HEAT_SET:
            _ctrl_heat_set(dbg, msg->u.ctrl_heat_set, state);
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

        // msgs with no effect on state
        case NETWORK_MSG_TYP_PUMP_REG_SET:
            _pump_reg_set(dbg, msg->u.pump_reg_set);
            break;
        case NETWORK_MSG_TYP_PUMP_REG_SET_RESP:
            _pump_reg_set_resp(dbg, msg->u.pump_reg_set_resp);
            break;
        case NETWORK_MSG_TYP_PUMP_CTRL:
            _pump_ctrl(dbg, msg->u.pump_ctrl);
            break;
        case NETWORK_MSG_TYP_CTRL_SET_ACK:  // response to various set requests
            _ctrl_set_ack(dbg, msg->u.ctrl_set_ack);
            break;
        case NETWORK_MSG_TYP_CTRL_CIRCUIT_SET:  // set circuit request (there appears to be no separate "get circuit request")
            _ctrl_circuit_set(dbg, msg->u.ctrl_circuit_set);
            break;

        // ignored msgs
        case NETWORK_MSG_TYP_CTRL_STATE_SET:
        case NETWORK_MSG_TYP_CTRL_SCHED_REQ:
        case NETWORK_MSG_TYP_CTRL_STATE_REQ:
        case NETWORK_MSG_TYP_CTRL_TIME_REQ:
        case NETWORK_MSG_TYP_CTRL_HEAT_REQ:
        case NETWORK_MSG_TYP_CTRL_LAYOUT_REQ:
        case NETWORK_MSG_TYP_CHLOR_PING_REQ:
        case NETWORK_MSG_TYP_CTRL_LAYOUT:
        case NETWORK_MSG_TYP_CTRL_LAYOUT_SET:
        case NETWORK_MSG_TYP_CHLOR_PING:
        case NETWORK_MSG_TYP_NONE:  // to please the gcc
            break;  //
    }
    if (CONFIG_POOL_DBG_POOLSTATE == 'y') {
        char const * const json = cJSON_Print(dbg);
        ESP_LOGI(TAG, "%s: %s", network_msg_typ_str(msg->typ), json);
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_DBG, json, ipc_for_dbg);
    }
    cJSON_Delete(dbg);

    bool const state_changed = memcmp(state, &old_state, sizeof(poolstate_t) != 0);
    if (state_changed) {
        poolstate_set(state);
    }
    return state_changed;
}
