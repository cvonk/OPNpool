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

cJSON *
_pump_reg_set(char const * const typ_str, mPumpRegulateSet_a5_t const * const msg)
{
    uint const address = (msg->addressHi << 8) | msg->addressLo;
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    cJSON * const root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, name_pump_prg(address), value);
    return root;
}

cJSON *
_pump_reg_set_resp(char const * const typ_str, mPumpRegulateSetResp_a5_t const * const msg)
{
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    cJSON * const root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "now", value);
    return root;
}

cJSON *
_pump_ctrl(char const * const typ_str, mPumpCtrl_a5_t const * const msg)
{
    char const * s;
    switch (msg->control) {
        case 0x00: s = "local"; break;
        case 0xFF: s = "remote"; break;
        default: s = name_hex8(msg->control);
    }
    cJSON * const root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "control", s);
    return root;
}

cJSON *
_pump_mode(char const * const typ_str, mPumpMode_a5_t const * const msg)
{
    cJSON * const root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "mode", name_pump_mode(msg->mode));
    return root;
}

cJSON *
_pump_state(char const * const typ_str, mPumpState_a5_t const * const msg)
{
    cJSON * const root = cJSON_CreateObject();
    cPool_AddPumpRunningToObject(root, "running", msg->state);
    return root;
}

cJSON *
_pump_status(char const * const typ_str, mPumpStatus_a5_t const * const msg, poolstate_pump_t * const pump_state)
{
	if (msg->state == 0x04 || msg->state == 0x0A) {

        pump_state->running = (msg->state == 0x0A);
        pump_state->mode = msg->mode;
        pump_state->status = msg->status;
        pump_state->pwr = ((uint16_t)msg->powerHi << 8) | msg->powerLo;
        pump_state->rpm = ((uint16_t)msg->rpmHi << 8) | msg->rpmLo;
        pump_state->err = msg->err;

        cJSON * const root = cJSON_CreateObject();
        cPool_AddPumpRunningToObject(root, "running", msg->state);

        cJSON_AddStringToObject(root, "mode", name_pump_mode(pump_state->mode));
        cJSON_AddNumberToObject(root, "status", pump_state->status);
        cJSON_AddNumberToObject(root, "pwr", pump_state->pwr);
        cJSON_AddNumberToObject(root, "rpm", pump_state->rpm);
        if (msg->gpm) {
            cJSON_AddNumberToObject(root, "gpm", msg->gpm);
        }
        if (msg->pct) {
            cJSON_AddNumberToObject(root, "pct", msg->pct);
        }
        cJSON_AddNumberToObject(root, "err", pump_state->mode);
        cJSON_AddNumberToObject(root, "timer", pump_state->mode);
        cJSON_AddStringToObject(root, "time", name_time(msg->hour, msg->minute));
        return root;
    }
    return NULL;
}

cJSON *
_ctrl_set_ack(char const * const typ_str, mCtrlSetAck_a5_t const * const msg)
{
    cJSON * const root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ack", name_datalink_a5_ctrl_msgtype(msg->typ));
    return root;
}

cJSON *
_ctrl_circuit_set(char const * const typ_str, mCtrlCircuitSet_a5_t const * const msg)
{
    cJSON * const root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, name_circuit(msg->circuit), msg->value);
    return root;
}

cJSON *
_ctrl_sched(char const * const typ_str, mCtrlSched_a5_t const * const msg, poolstate_sched_t * state_sched, uint const state_sched_len)
{
    cJSON * const root = cJSON_CreateObject();

    for (uint  ii = 0; ii < state_sched_len; ii++) {

        mCtrlSchedSub_a5_t * p = (mCtrlSchedSub_a5_t *)&msg->sched[ii];
        uint16_t const start = (uint16_t)p->prgStartHi << 8 | p->prgStartLo;
        uint16_t const stop = (uint16_t)p->prgStopHi << 8 | p->prgStopLo;

        state_sched[ii].circuit = p->circuit;
        state_sched[ii].start = start;
        state_sched[ii].stop = stop;

        cJSON * const obj = cJSON_CreateObject();
        cJSON_AddItemToObject(root, name_circuit(p->circuit), obj);
        cJSON_AddStringToObject(obj, "start", name_time(start / 60, start % 60));
        cJSON_AddStringToObject(obj, "stop", name_time(stop / 60, stop % 60));
    }
    return root;
}

cJSON *
_ctrl_state(char const * const typ_str, mCtrlState_a5_t const * const msg, poolstate_t * state)
{
    cJSON * const root = cJSON_CreateObject();

    cPool_AddSystemToObject(root, "system", msg->hour, msg->minute, msg->major + (float)msg->minor / 100);
    cPool_AddActiveCircuitsToObject(root, "active", (msg->activeHi << 8) | msg->activeLo);
    cPool_AddThermostatToObject(root, "pool", msg->poolTemp, name_heat_src(msg->heatSrc & 0x03), msg->heating & 0x04);
    cPool_AddThermostatToObject(root, "spa", msg->spaTemp, name_heat_src(msg->heatSrc >> 2), msg->heating & 0x08);
    cPool_AddThermostatToObject(root, "air", msg->airTemp, NULL, false);
    if (msg->solarTemp != 0xFF) {
        cPool_AddThermostatToObject(root, "solar", msg->solarTemp, NULL, false);
    }
    return root;
}

bool
poolstate_receive_update(network_msg_t const * const msg, poolstate_t * const state)
{
    cJSON * root = NULL;
    char const * const typ_str = name_network_msgtyp(msg->typ);

    poolstate_t old_state;
    memcpy(&old_state, state, sizeof(poolstate_t));

    switch(msg->typ) {
        case NETWORK_MSGTYP_PUMP_REG_SET:
            root = _pump_reg_set(typ_str, msg->u.pump_reg_set);
            break;
        case NETWORK_MSGTYP_PUMP_REG_SET_RESP:
            root = _pump_reg_set_resp(typ_str, msg->u.pump_reg_set_resp);
            break;
        case NETWORK_MSGTYP_PUMP_CTRL:
            root = _pump_ctrl(typ_str, msg->u.pump_ctrl);
            break;
        case NETWORK_MSGTYP_PUMP_MODE:
            root = _pump_mode(typ_str, msg->u.pump_mode);
            break;
        case NETWORK_MSGTYP_PUMP_STATE:
            root = _pump_state(typ_str, msg->u.pump_state);
            break;
        case NETWORK_MSGTYP_PUMP_STATUS_REQ:
             break;
        case NETWORK_MSGTYP_PUMP_STATUS:
            root = _pump_status(typ_str, msg->u.pump_status, &state->pump);
            break;

        // response to various set requests
        case NETWORK_MSGTYP_CTRL_SET_ACK:
            root = _ctrl_set_ack(typ_str, msg->u.ctrl_set_ack);
            break;

        // set circuit request (there appears to be no separate "get circuit request")
        case NETWORK_MSGTYP_CTRL_CIRCUIT_SET:
            root = _ctrl_circuit_set(typ_str, msg->u.ctrl_circuit_set);
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
            root = _ctrl_sched(typ_str, msg->u.ctrl_sched, state->sched, ARRAY_SIZE(state->sched));
            break;

            // state: get response / set request
        case NETWORK_MSGTYP_CTRL_STATE:
            root = _ctrl_state(typ_str, msg->u.ctrl_state, state);
            break;

        case NETWORK_MSGTYP_CTRL_STATE_SET:
        case NETWORK_MSGTYP_CTRL_TIME:
        case NETWORK_MSGTYP_CTRL_TIME_SET:
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
    if (root) {
        char * const buf = cJSON_Print(root);
        cJSON_Delete(root);
        ESP_LOGI(TAG, "%s: %s", typ_str, buf);
        free(buf);
    } else {
        ESP_LOGI(TAG, "%s", typ_str);
    }
    return memcmp(&old_state, state, sizeof(poolstate_t)) != 0;
}
