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

#include "../network/network.h"
#include "../network/name.h"
#include "poolstate.h"
#include "to_json.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "receive_updates";

void
_pump_reg_set(cJSON * const root, mPumpRegulateSet_a5_t const * const msg)
{
    uint const address = (msg->addressHi << 8) | msg->addressLo;
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    cPool_AddPumpPrgToObject(root, name_pump_prg(address), value);
}

void
_pump_reg_set_resp(cJSON * const root, mPumpRegulateSetResp_a5_t const * const msg)
{
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    cPool_AddPumpPrgToObject(root, "now", value);
}


void
_pump_ctrl(cJSON * const root, mPumpCtrl_a5_t const * const msg)
{
    cPool_AddPumpCtrlToObject(root, "ctrl", msg->ctrl);
}

void
_pump_running(cJSON * const root, mPumpRunning_a5_t const * const msg, poolstate_t * const state)
{
    bool const running = msg->running == 0x0A;
    bool const not_running = msg->running == 0x04;
    if (!running && !not_running) {
        return;
    }
    state->pump.running = running;
    cPool_AddPumpRunningToObject(root, "running", state->pump.running);
}

void
_pump_mode(cJSON * const root, mPumpMode_a5_t const * const msg, poolstate_t * const state)
{
    state->pump.mode = msg->mode;

    cPool_AddPumpModeToObject(root, "mode", msg->mode);
}

void
_pump_status(cJSON * const root, mPumpStatus_a5_t const * const msg, poolstate_t * const state)
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

    cPool_AddPumpStatusToObject(root, "status", &state->pump);
}

void
_ctrl_set_ack(cJSON * const root, mCtrlSetAck_a5_t const * const msg)
{
    cJSON_AddNumberToObject(root, "ack", msg->typ);
}

void
_ctrl_circuit_set(cJSON * const root, mCtrlCircuitSet_a5_t const * const msg)
{
    cJSON_AddNumberToObject(root, name_circuit(msg->circuit), msg->value);
}

void
_ctrl_sched(cJSON * const root, mCtrlSched_a5_t const * const msg, poolstate_t * state)
{
    mCtrlSchedSub_a5_t const * msg_sched = msg->scheds;
    poolstate_sched_t * state_sched = state->scheds;

    cJSON * const item = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "schedules", item);

    for (uint  ii = 0; ii < ARRAY_SIZE(state->scheds); ii++, msg_sched++, state_sched++) {

        state_sched->circuit = msg_sched->circuit;
        state_sched->start = (uint16_t)msg_sched->prgStartHi << 8 | msg_sched->prgStartLo;
        state_sched->stop = (uint16_t)msg_sched->prgStopHi << 8 | msg_sched->prgStopLo;

        cPool_AddSchedToObject(item, name_circuit(state_sched->circuit), state_sched->start, state_sched->stop);
    }
}

void
_ctrl_state(cJSON * const root, mCtrlState_a5_t const * const msg, poolstate_t * state)
{
    state->tod.time.minute = msg->minute;
    state->tod.time.hour = msg->hour;
    state->version.major = msg->major;
    state->version.minor = msg->minor;
    state->circuits.active = (msg->activeHi << 8) | msg->activeLo;
    state->circuits.delay = msg->delayLo;
    state->pool.temp = msg->poolTemp;
    state->pool.heat_src = msg->heatSrc & 0x03;
    state->pool.heating = msg->heating & 0x04;
    state->spa.temp = msg->poolTemp;
    state->spa.heat_src = msg->heatSrc >> 2;
    state->spa.heating = msg->heating & 0x08;
    state->air.temp = msg->airTemp;
    state->solar.temp = msg->solarTemp;

    cPool_AddStateToObject(root, "state", state);
}

void
_ctrl_time(cJSON * const root, mCtrlTime_a5_t const * const msg, poolstate_t * const state)
{
    state->tod.time.minute = msg->minute;
    state->tod.time.hour = msg->hour;
    state->tod.date.day = msg->day;
    state->tod.date.month = msg->month;
    state->tod.date.year = msg->year;

    cPool_AddTodToObject(root, "tod", &state->tod);
}

bool
poolstate_receive_update(network_msg_t const * const msg, poolstate_t * const state)
{
    poolstate_t old_state;
    memcpy(&old_state, state, sizeof(poolstate_t));

    cJSON * const root = cJSON_CreateObject();
    switch(msg->typ) {
        case NETWORK_MSGTYP_PUMP_REG_SET:
            _pump_reg_set(root, msg->u.pump_reg_set);
            break;
        case NETWORK_MSGTYP_PUMP_REG_SET_RESP:
            _pump_reg_set_resp(root, msg->u.pump_reg_set_resp);
            break;
        case NETWORK_MSGTYP_PUMP_CTRL:
            _pump_ctrl(root, msg->u.pump_ctrl);
            break;
        case NETWORK_MSGTYP_PUMP_MODE:
            _pump_mode(root, msg->u.pump_mode, state);
            break;
        case NETWORK_MSGTYP_PUMP_RUNNING:
            _pump_running(root, msg->u.pump_running, state);
            break;
        case NETWORK_MSGTYP_PUMP_STATUS_REQ:
             break;
        case NETWORK_MSGTYP_PUMP_STATUS:
            _pump_status(root, msg->u.pump_status, state);
            break;

        // response to various set requests
        case NETWORK_MSGTYP_CTRL_SET_ACK:
            _ctrl_set_ack(root, msg->u.ctrl_set_ack);
            break;

        // set circuit request (there appears to be no separate "get circuit request")
        case NETWORK_MSGTYP_CTRL_CIRCUIT_SET:
            _ctrl_circuit_set(root, msg->u.ctrl_circuit_set);
            break;

        // various get requests
        case NETWORK_MSGTYP_CTRL_SCHED_REQ:
        case NETWORK_MSGTYP_CTRL_STATE_REQ:
        case NETWORK_MSGTYP_CTRL_TIME_REQ:
        case NETWORK_MSGTYP_CTRL_HEAT_REQ:
        case NETWORK_MSGTYP_CTRL_LAYOUT_REQ:
            break;

        // schedule: get response / set request
        case NETWORK_MSGTYP_CTRL_SCHED:
            _ctrl_sched(root, msg->u.ctrl_sched, state);
            break;

            // state: get response / set request
        case NETWORK_MSGTYP_CTRL_STATE:
            _ctrl_state(root, msg->u.ctrl_state, state);
            break;

        case NETWORK_MSGTYP_CTRL_STATE_SET:
            // 2BD
            break;

        case NETWORK_MSGTYP_CTRL_TIME:
        case NETWORK_MSGTYP_CTRL_TIME_SET:
            _ctrl_time(root, msg->u.ctrl_time, state);
            break;


        case NETWORK_MSGTYP_CTRL_HEAT:
        case NETWORK_MSGTYP_CTRL_HEAT_SET:
        case NETWORK_MSGTYP_CTRL_LAYOUT:
        case NETWORK_MSGTYP_CTRL_LAYOUT_SET:
        case NETWORK_MSGTYP_CHLOR_PING_REQ:
        case NETWORK_MSGTYP_CHLOR_PING:
        case NETWORK_MSGTYP_CHLOR_NAME:
        case NETWORK_MSGTYP_CHLOR_LEVEL_SET:
        case NETWORK_MSGTYP_CHLOR_LEVEL_RESP:
            break;
        case NETWORK_MSGTYP_NONE:
            break;  //  to make the compiler happy
    }
    char * const buf = cJSON_Print(root);
    cJSON_Delete(root);
    ESP_LOGI(TAG, "%s: %s", name_network_msgtyp(msg->typ), buf);
    free(buf);
    return memcmp(&old_state, state, sizeof(poolstate_t)) != 0;
}
