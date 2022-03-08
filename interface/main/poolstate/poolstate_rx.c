/**
 * @brief Pool state: updates the state in response to network messages
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2022, Coert Vonk
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

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddTodToObject(dbg, "tod", &state->system.tod);
    }
}

static void
_ctrl_heat_resp(cJSON * const dbg, network_msg_ctrl_heat_resp_t const * const msg, poolstate_t * const state)
{
    state->thermos[POOLSTATE_THERMO_TYP_POOL].temp = msg->poolTemp;
    state->thermos[POOLSTATE_THERMO_TYP_POOL].set_point = msg->poolSetpoint;
    state->thermos[POOLSTATE_THERMO_TYP_POOL].heat_src = msg->heatSrc & 0x03;
    state->thermos[POOLSTATE_THERMO_TYP_SPA].temp = msg->spaTemp;
    state->thermos[POOLSTATE_THERMO_TYP_SPA].set_point = msg->spaSetpoint;
    state->thermos[POOLSTATE_THERMO_TYP_SPA].heat_src = msg->heatSrc >> 2;

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddThermosToObject(dbg, "thermos", state->thermos, true, true, true, false);
    }
}

static void
_ctrl_heat_set(cJSON * const dbg, network_msg_ctrl_heat_set_t const * const msg, poolstate_t * const state)
{
    state->thermos[POOLSTATE_THERMO_TYP_POOL].set_point = msg->poolSetpoint;
    state->thermos[POOLSTATE_THERMO_TYP_POOL].heat_src = msg->heatSrc & 0x03;
    state->thermos[POOLSTATE_THERMO_TYP_SPA].set_point = msg->spaSetpoint;
    state->thermos[POOLSTATE_THERMO_TYP_SPA].heat_src = msg->heatSrc >> 2;

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddThermosToObject(dbg, "thermos", state->thermos, false, true, true, false);
    }
}

static void
_ctrl_hex_bytes(cJSON * const dbg, uint8_t const * const bytes, poolstate_t * const state, uint8_t nrBytes)
{
    char const * str[nrBytes];

    for (uint8_t ii = 0; ii < nrBytes; ii++) {
        str[ii] = hex8_str(bytes[ii]);
    }
    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON * const array = cJSON_CreateStringArray(str, nrBytes);
        cJSON_AddItemToObject(dbg, "raw", array);
    }
}

static void
_ctrl_circuit_set(cJSON * const dbg, network_msg_ctrl_circuit_set_t const * const msg, poolstate_t * const state)
{
    state->circuits.active[msg->circuit -1] = msg->value;

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddNumberToObject(dbg, network_circuit_str(msg->circuit -1), msg->value);
    }
}

// 2B: sched should be its own entity instead of part of thermos
static void
_ctrl_sched_resp(cJSON * const dbg, network_msg_ctrl_sched_resp_t const * const msg, poolstate_t * const state)
{
#if 1
    network_msg_ctrl_sched_resp_sub_t const * msg_sched = msg->scheds;
    poolstate_sched_t * state_sched = state->scheds;

    for (uint ii = 0; ii < POOLSTATE_SCHED_TYP_COUNT; ii++, msg_sched++, state_sched++) {
        state_sched->circuit = msg_sched->circuit - 1;
        state_sched->start = (uint16_t)msg_sched->prgStartHi << 8 | msg_sched->prgStartLo;
        state_sched->stop = (uint16_t)msg_sched->prgStopHi << 8 | msg_sched->prgStopLo;
    }
    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddSchedsToObject(dbg, "scheds", state->scheds, true);
    }
#else    
    network_msg_ctrl_sched_resp_sub_t const * msg_sched = msg->scheds;

    for (uint ii = 0; ii < POOLSTATE_THERMO_TYP_COUNT; ii++, msg_sched++) {

        if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
            ESP_LOGI(TAG, "%s[%u], circuit=%s", __FUNCTION__, ii, network_circuit_str(msg_sched->circuit));
        }
        uint16_t const start = (uint16_t)msg_sched->prgStartHi << 8 | msg_sched->prgStartLo;
        uint16_t const stop = (uint16_t)msg_sched->prgStopHi << 8 | msg_sched->prgStopLo;

        if (msg_sched->circuit == NETWORK_CIRCUIT_POOL + 1) {
            state->thermos[POOLSTATE_THERMO_TYP_POOL].sched.start = start;
            state->thermos[POOLSTATE_THERMO_TYP_POOL].sched.stop = stop;
        } else if (msg_sched->circuit == NETWORK_CIRCUIT_SPA + 1) {
            state->thermos[POOLSTATE_THERMO_TYP_SPA].sched.start = start;
            state->thermos[POOLSTATE_THERMO_TYP_SPA].sched.stop = stop;
        }
    }
    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddThermosToObject(dbg, "thermos", state->thermos, false, false, false, false, true);
    }
#endif
}

static void
_ctrl_state(cJSON * const dbg, network_msg_ctrl_state_bcast_t const * const msg, poolstate_t * state)
{
    // update state->circuits.active
    bool * state_active = state->circuits.active;
    uint16_t msg_mask = 0x00001;
    uint16_t const msg_active = ((uint16_t)msg->activeHi << 8) | msg->activeLo;
    for (uint ii = 0; ii < NETWORK_CIRCUIT_COUNT; ii++, state_active++) {
        *state_active = msg_active & msg_mask;
        msg_mask <<= 1;
    }
    // if both SPA and POOL bits are set, only SPA runs
    if (state->circuits.active[NETWORK_CIRCUIT_SPA]) {
        state->circuits.active[NETWORK_CIRCUIT_POOL] = false;
    }

    // update state->circuits.delay
    bool * state_delay = state->circuits.delay;
    msg_mask = 0x00001;
    for (uint ii = 0; ii < NETWORK_CIRCUIT_COUNT; ii++, state_delay++) {
        *state_delay = msg->delay & msg_mask;
        msg_mask <<= 1;
    }

    // update state->circuits.thermos (only update when the pump is running)
    if (state->circuits.active[NETWORK_CIRCUIT_SPA]) {
        state->thermos[POOLSTATE_THERMO_TYP_SPA].temp = msg->poolTemp;
    }
    if (state->circuits.active[NETWORK_CIRCUIT_POOL]) {
        state->thermos[POOLSTATE_THERMO_TYP_POOL].temp = msg->poolTemp;
    }

    state->thermos[POOLSTATE_THERMO_TYP_POOL].heating = msg->heatStatus & 0x04;
    state->thermos[POOLSTATE_THERMO_TYP_SPA].heating = msg->heatStatus & 0x08;

    state->thermos[POOLSTATE_THERMO_TYP_POOL].heat_src = msg->heatSrc & 0x03;  // NETWORK_HEAT_SRC_*
    state->thermos[POOLSTATE_THERMO_TYP_SPA].heat_src = msg->heatSrc >> 2;     // NETWORK_HEAT_SRC_*

    // update state->circuits.modes
    bool * state_mode = state->modes.set;
    msg_mask = 0x00001;
    for (uint ii = 0; ii < NETWORK_MODE_COUNT; ii++, state_mode++) {
        *state_mode = msg->mode & msg_mask;
        msg_mask <<= 1;
    }

    // update state->system (date is updated through `network_msg_ctrl_time`)
    state->system.tod.time.minute = msg->minute;
    state->system.tod.time.hour = msg->hour;

    // update state->temps
    state->temps[POOLSTATE_TEMP_TYP_AIR].temp = msg->airTemp;
    state->temps[POOLSTATE_TEMP_TYP_SOLAR].temp = msg->solarTemp;

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddStateToObject(dbg, "state", state);
    }
}

/**
 * version
 */

static void
_ctrl_version_resp(cJSON * const dbg, network_msg_ctrl_version_resp_t const * const msg, poolstate_t * state)
{
    state->system.version.major = msg->major;
    state->system.version.minor = msg->minor;

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddVersionToObject(dbg, "firmware", &state->system.version);
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

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddPumpPrgToObject(dbg, network_pump_prg_str(address), value);
    }
}

static void
_pump_reg_set_resp(cJSON * const dbg, network_msg_pump_reg_resp_t const * const msg)
{
    uint const value = (msg->valueHi << 8) | msg->valueLo;

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddPumpPrgToObject(dbg, "resp", value);
    }
}


static void
_pump_ctrl(cJSON * const dbg, network_msg_pump_ctrl_t const * const msg)
{
    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
       cJSON_AddPumpCtrlToObject(dbg, "ctrl", msg->ctrl);
    }
}

static void
_pump_mode(cJSON * const dbg, network_msg_pump_mode_t const * const msg, poolstate_t * const state)
{
    state->pump.mode = msg->mode;

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
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
    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddPumpRunningToObject(dbg, "running", state->pump.running);
    }
}

static void
_pump_status(cJSON * const dbg, network_msg_pump_status_resp_t const * const msg, poolstate_t * const state)
{
    bool const running = msg->running == 0x0A;
    bool const not_running = msg->running == 0x04;
    if (!running && !not_running) {
        return;
    }
    state->pump.running = running;
    state->pump.mode = msg->mode;
    state->pump.state = msg->state;
    state->pump.pwr = ((uint16_t)msg->powerHi << 8) | msg->powerLo;
    state->pump.rpm = ((uint16_t)msg->rpmHi << 8) | msg->rpmLo;
    state->pump.gpm = msg->gpm;
    state->pump.pct = msg->pct;
    state->pump.err = msg->err;
    state->pump.timer = msg->timer;
    state->pump.time.hour = msg->hour;
    state->pump.time.minute = msg->minute;

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
       cJSON_AddPumpToObject(dbg, "status", state);
    }
}

static void
_ctrl_set_ack(cJSON * const dbg, network_msg_ctrl_set_ack_t const * const msg)
{
    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddNumberToObject(dbg, "ack", msg->typ);
    }
}

/**
 * chlor
 **/

static void
_chlor_name_resp(cJSON * const dbg, network_msg_chlor_name_resp_t const * const msg, poolstate_t * const state)
{
    state->chlor.salt = (uint16_t)msg->salt * 50;

    size_t name_size = sizeof(state->chlor.name);
    strncpy(state->chlor.name, msg->name, name_size);
    state->chlor.name[name_size - 1] = '\0';

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddNumberToObject(dbg, "salt", state->chlor.salt);
        cJSON_AddStringToObject(dbg, "name", state->chlor.name);
    }
}

static void
_chlor_level_set(cJSON * const dbg, network_msg_chlor_level_set_t const * const msg, poolstate_t * const state)
{
    state->chlor.pct = msg->pct;

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddNumberToObject(dbg, "pct", state->chlor.pct);
    }
}

static void
_chlor_level_set_resp(cJSON * const dbg, network_msg_chlor_level_resp_t const * const msg, poolstate_t * const state)
{
    state->chlor.salt = (uint16_t)msg->salt * 50;
    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        //ESP_LOGD(TAG, "%s salt=%u, status=0x%02X", __func__, msg->salt, msg->err);
    }
    if (msg->err & 0x01) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_LOW_FLOW;
    } else if (msg->err & 0x02) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_LOW_SALT;
    } else if (msg->err & 0x04) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_HIGH_SALT;
    } else if (msg->err & 0x10) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_CLEAN_CELL;
    } else if (msg->err & 0x40) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_COLD;
    } else if (msg->err & 0x80) {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_OK;
    } else {
        state->chlor.status = POOLSTATE_CHLOR_STATUS_OTHER;
    }
    // good salt range is 2600 to 4500 ppm

    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        cJSON_AddChlorRespToObject(dbg, "chlor", &state->chlor);
    }
}

esp_err_t
poolstate_rx_update(network_msg_t const * const msg, poolstate_t * const state, ipc_t const * const ipc_for_dbg)
{
	name_reset_idx();

    poolstate_t old_state;
    (void)poolstate_get(&old_state);
    memcpy(state, &old_state, sizeof(poolstate_t));

    cJSON * const dbg = cJSON_CreateObject();
    switch (msg->typ) {
        case MSG_TYP_CTRL_SET_ACK:  // response to various set requests
            _ctrl_set_ack(dbg, msg->u.ctrl_set_ack);
            break;
        case MSG_TYP_CTRL_CIRCUIT_SET:
            _ctrl_circuit_set(dbg, msg->u.ctrl_circuit_set, state);
            break;
        case MSG_TYP_CTRL_SCHED_REQ:
            break;
        case MSG_TYP_CTRL_SCHED_RESP:
            _ctrl_sched_resp(dbg, msg->u.ctrl_sched_resp, state);
            break;
        case MSG_TYP_CTRL_STATE_BCAST:
            _ctrl_state(dbg, msg->u.ctrl_state, state);
            break;
        case MSG_TYP_CTRL_TIME_REQ:
            break;
        case MSG_TYP_CTRL_TIME_RESP:
            _ctrl_time(dbg, msg->u.ctrl_time_resp, state);
            break;
        case MSG_TYP_CTRL_TIME_SET:
            _ctrl_time(dbg, msg->u.ctrl_time_set, state);
            break;
        case MSG_TYP_CTRL_HEAT_REQ:
            break;
        case MSG_TYP_CTRL_HEAT_RESP:
            _ctrl_heat_resp(dbg, msg->u.ctrl_heat_resp, state);
            break;
        case MSG_TYP_CTRL_HEAT_SET:
            _ctrl_heat_set(dbg, msg->u.ctrl_heat_set, state);
            break;
        case MSG_TYP_CTRL_LAYOUT_REQ:
        case MSG_TYP_CTRL_LAYOUT:
        case MSG_TYP_CTRL_LAYOUT_SET:
        case MSG_TYP_CTRL_VALVE_REQ:
        case MSG_TYP_CTRL_SOLARPUMP_REQ:
        case MSG_TYP_CTRL_DELAY_REQ:
        case MSG_TYP_CTRL_HEAT_SETPT_REQ:
        case MSG_TYP_CTRL_VERSION_REQ:
            break;
        case MSG_TYP_CTRL_SCHEDS_REQ:
        case MSG_TYP_CTRL_CIRC_NAMES_REQ:
        case MSG_TYP_CTRL_UNKN_D2_REQ:
        case MSG_TYP_CHLOR_NAME_REQ:
            _ctrl_hex_bytes(dbg, msg->u.bytes, state, 1);
            break;
        case MSG_TYP_CTRL_VERSION_RESP:
            _ctrl_version_resp(dbg, msg->u.ctrl_version_resp, state);
            break;
        case MSG_TYP_CTRL_VALVE_RESP:
            _ctrl_hex_bytes(dbg, msg->u.bytes, state, sizeof(network_msg_ctrl_valve_resp_t));
            break;
        case MSG_TYP_CTRL_SOLARPUMP_RESP:
            _ctrl_hex_bytes(dbg, msg->u.bytes, state, sizeof(network_msg_ctrl_solarpump_resp_t));
            break;
        case MSG_TYP_CTRL_DELAY_RESP:
            _ctrl_hex_bytes(dbg, msg->u.bytes, state, sizeof(network_msg_ctrl_delay_resp_t));
            break;
        case MSG_TYP_CTRL_HEAT_SETPT_RESP:
            _ctrl_hex_bytes(dbg, msg->u.bytes, state, sizeof(network_msg_ctrl_heat_setpt_resp_t));
            break;
        case MSG_TYP_CTRL_CIRC_NAMES_RESP:
            _ctrl_hex_bytes(dbg, msg->u.bytes, state, sizeof(network_msg_ctrl_circ_names_resp_t));
            break;
        case MSG_TYP_CTRL_SCHEDS_RESP:
            _ctrl_hex_bytes(dbg, msg->u.bytes, state, sizeof(network_msg_ctrl_scheds_resp_t));
            break;
        case MSG_TYP_PUMP_REG_SET:
            _pump_reg_set(dbg, msg->u.pump_reg_set);
            break;
        case MSG_TYP_PUMP_REG_RESP:
            _pump_reg_set_resp(dbg, msg->u.pump_reg_set_resp);
            break;
        case MSG_TYP_PUMP_CTRL_SET:
        case MSG_TYP_PUMP_CTRL_RESP:
            _pump_ctrl(dbg, msg->u.pump_ctrl);
            break;
        case MSG_TYP_PUMP_MODE_SET:
        case MSG_TYP_PUMP_MODE_RESP:
            _pump_mode(dbg, msg->u.pump_mode, state);
            break;
        case MSG_TYP_PUMP_RUN_SET:
        case MSG_TYP_PUMP_RUN_RESP:
            _pump_run(dbg, msg->u.pump_run, state);
            break;
        case MSG_TYP_PUMP_STATUS_REQ:
             break;
        case MSG_TYP_PUMP_STATUS_RESP:
            _pump_status(dbg, msg->u.pump_status_resp, state);
            break;
        case MSG_TYP_CHLOR_PING_REQ:
        case MSG_TYP_CHLOR_PING_RESP:
            break;
        case MSG_TYP_CHLOR_NAME_RESP:
            _chlor_name_resp(dbg, msg->u.chlor_name_resp, state);
            break;
        case MSG_TYP_CHLOR_LEVEL_SET:
            _chlor_level_set(dbg, msg->u.chlor_level_set, state);
            break;
        case MSG_TYP_CHLOR_LEVEL_RESP:
            _chlor_level_set_resp(dbg, msg->u.chlor_level_resp, state);
            break;
        case MSG_TYP_NONE:  // to please the gcc
            break;  //
    }
    if (CONFIG_POOL_DBGLVL_POOLSTATE > 1) {
        size_t const json_size = 1024;
        char * const json = malloc(json_size);
        assert( cJSON_PrintPreallocated(dbg, json, json_size, false) );
        ESP_LOGI(TAG, "{%s: %s}\n", network_msg_typ_str(msg->typ), json);
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_DBG, json, ipc_for_dbg);
        free(json);
    }
    cJSON_Delete(dbg);

    bool const state_changed = memcmp(state, &old_state, sizeof(poolstate_t)) != 0;
    if (state_changed) {
        poolstate_set(state);
    }
    return state_changed ? ESP_OK : ESP_FAIL;
}
