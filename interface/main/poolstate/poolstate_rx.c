/**
 * @brief Pool state and access functions
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

//#include "../utils/utils_str.h"
#include "../network/network.h"
#include "poolstate.h"
#include "cpool.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "receive_updates";

void
_pump_reg_set(cJSON * const dbg, mPumpRegulateSet_a5_t const * const msg)
{
    uint const address = (msg->addressHi << 8) | msg->addressLo;
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    cPool_AddPumpPrgToObject(dbg, network_pump_prg_str(address), value);
}

void
_pump_reg_set_resp(cJSON * const dbg, mPumpRegulateSetResp_a5_t const * const msg)
{
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    cPool_AddPumpPrgToObject(dbg, "now", value);
}


void
_pump_ctrl(cJSON * const dbg, mPumpCtrl_a5_t const * const msg)
{
    cPool_AddPumpCtrlToObject(dbg, "ctrl", msg->ctrl);
}

void
_pump_running(cJSON * const dbg, mPumpRunning_a5_t const * const msg, poolstate_t * const state)
{
    bool const running = msg->running == 0x0A;
    bool const not_running = msg->running == 0x04;
    if (!running && !not_running) {
        return;
    }
    state->pump.running = running;
    cPool_AddPumpRunningToObject(dbg, "running", state->pump.running);
}

void
_pump_mode(cJSON * const dbg, mPumpMode_a5_t const * const msg, poolstate_t * const state)
{
    state->pump.mode = msg->mode;

    cPool_AddPumpModeToObject(dbg, "mode", msg->mode);
}

void
_pump_status(cJSON * const dbg, mPumpStatus_a5_t const * const msg, poolstate_t * const state)
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

    cPool_AddPumpStatusToObject(dbg, "status", &state->pump);
}

void
_ctrl_set_ack(cJSON * const dbg, mCtrlSetAck_a5_t const * const msg)
{
    cJSON_AddNumberToObject(dbg, "ack", msg->typ);
}

void
_ctrl_circuit_set(cJSON * const dbg, mCtrlCircuitSet_a5_t const * const msg)
{
    cJSON_AddNumberToObject(dbg, network_circuit_str(msg->circuit), msg->value);
}

void
_ctrl_sched(cJSON * const dbg, mCtrlSched_a5_t const * const msg, poolstate_t * state)
{
    mCtrlSchedSub_a5_t const * msg_sched = msg->scheds;
    poolstate_sched_t * state_sched = state->scheds;

    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(dbg, "schedules", item);

    for (uint  ii = 0; ii < ARRAY_SIZE(state->scheds); ii++, msg_sched++, state_sched++) {

        state_sched->circuit = msg_sched->circuit;
        state_sched->start = (uint16_t)msg_sched->prgStartHi << 8 | msg_sched->prgStartLo;
        state_sched->stop = (uint16_t)msg_sched->prgStopHi << 8 | msg_sched->prgStopLo;

        cPool_AddSchedToObject(item, network_circuit_str(state_sched->circuit), state_sched->start, state_sched->stop);
    }
}

void
_ctrl_state(cJSON * const dbg, mCtrlState_a5_t const * const msg, poolstate_t * state)
{
#if 0
    uint len = 0;
    uint buf_size = 80;
    char buf[buf_size];

    uint8_t const * const asBytes = (uint8_t const * const) msg;
    for (uint ii = 0; ii < sizeof(mCtrlState_a5_t); ii++) {
        len += snprintf(buf + len, buf_size - len, " %02X", asBytes[ii]);
    }
#endif

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

    cPool_AddStateToObject(dbg, "state", state);
}

void
_ctrl_time(cJSON * const dbg, mCtrlTime_a5_t const * const msg, poolstate_t * const state)
{
    state->tod.time.minute = msg->minute;
    state->tod.time.hour = msg->hour;
    state->tod.date.day = msg->day;
    state->tod.date.month = msg->month;
    state->tod.date.year = msg->year;

    cPool_AddTodToObject(dbg, "tod", &state->tod);
}

bool
poolstate_rx_update(network_msg_t const * const msg, poolstate_t * const state, char * * const json)
{
    poolstate_t old_state;
    poolstate_get(&old_state);
    memcpy(state, &old_state, sizeof(poolstate_t));

    cJSON * const dbg = cJSON_CreateObject();
    switch(msg->typ) {
        case NETWORK_MSG_TYP_PUMP_REG_SET:
            _pump_reg_set(dbg, msg->u.pump_reg_set);
            break;
        case NETWORK_MSG_TYP_PUMP_REG_SET_RESP:
            _pump_reg_set_resp(dbg, msg->u.pump_reg_set_resp);
            break;
        case NETWORK_MSG_TYP_PUMP_CTRL:
            _pump_ctrl(dbg, msg->u.pump_ctrl);
            break;
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

        // response to various set requests
        case NETWORK_MSG_TYP_CTRL_SET_ACK:
            _ctrl_set_ack(dbg, msg->u.ctrl_set_ack);
            break;

        // set circuit request (there appears to be no separate "get circuit request")
        case NETWORK_MSG_TYP_CTRL_CIRCUIT_SET:
            _ctrl_circuit_set(dbg, msg->u.ctrl_circuit_set);
            break;

        // various get requests
        case NETWORK_MSG_TYP_CTRL_SCHED_REQ:
        case NETWORK_MSG_TYP_CTRL_STATE_REQ:
        case NETWORK_MSG_TYP_CTRL_TIME_REQ:
        case NETWORK_MSG_TYP_CTRL_HEAT_REQ:
        case NETWORK_MSG_TYP_CTRL_LAYOUT_REQ:
            break;

        // schedule: get response / set request
        case NETWORK_MSG_TYP_CTRL_SCHED:
            _ctrl_sched(dbg, msg->u.ctrl_sched, state);
            break;

            // state: get response / set request
        case NETWORK_MSG_TYP_CTRL_STATE:
            _ctrl_state(dbg, msg->u.ctrl_state, state);
            break;

        case NETWORK_MSG_TYP_CTRL_STATE_SET:
            // 2BD
            break;

        case NETWORK_MSG_TYP_CTRL_TIME:
        case NETWORK_MSG_TYP_CTRL_TIME_SET:
            _ctrl_time(dbg, msg->u.ctrl_time, state);
            break;


        case NETWORK_MSG_TYP_CTRL_HEAT:
        case NETWORK_MSG_TYP_CTRL_HEAT_SET:
        case NETWORK_MSG_TYP_CTRL_LAYOUT:
        case NETWORK_MSG_TYP_CTRL_LAYOUT_SET:
        case NETWORK_MSG_TYP_CHLOR_PING_REQ:
        case NETWORK_MSG_TYP_CHLOR_PING:
        case NETWORK_MSG_TYP_CHLOR_NAME:
        case NETWORK_MSG_TYP_CHLOR_LEVEL_SET:
        case NETWORK_MSG_TYP_CHLOR_LEVEL_RESP:
            break;
        case NETWORK_MSG_TYP_NONE:
            break;  //  to make the compiler happy
    }
    *json = cJSON_Print(dbg);
    cJSON_Delete(dbg);
    ESP_LOGI(TAG, "%s: %s", network_msg_typ_str(msg->typ), *json);

    bool const state_changed = memcmp(state, &old_state, sizeof(poolstate_t));
    if (state_changed) {
        poolstate_set(state);
    }
    return state_changed;  // caller must ALWAYS free(*json)
}
