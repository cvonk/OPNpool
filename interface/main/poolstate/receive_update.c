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

static char const * const TAG = "receive_updates";

void
_pump_reg_set(char const * const typ_str, mPumpRegulateSet_a5_t const * const msg)
{
    uint const address = (msg->addressHi << 8) | msg->addressLo;
    uint const value = (msg->valueHi << 8) | msg->valueLo;
    ESP_LOGI(TAG, "\"%s\": {\"%s\": %u}", typ_str, name_pump_prg(address), value);
}

void
_pump_reg_set_resp(char const * const typ_str, mPumpRegulateSetResp_a5_t const * const msg)
{
    uint const value = (msg->valueHi << 8) | msg->valueLo;
    ESP_LOGI(TAG, "\"%s\": {\"now\": %u}", typ_str, value);
}

void
_pump_ctrl(char const * const typ_str, mPumpCtrl_a5_t const * const msg)
{
    char const * s;
    switch (msg->control) {
        case 0x00: s = "local"; break;
        case 0xFF: s = "remote"; break;
        default: s = name_hex8(msg->control);
    }
    ESP_LOGI(TAG, "\"%s\": {\"control\": %s}", typ_str, s);
}

void
_pump_mode(char const * const typ_str, mPumpMode_a5_t const * const msg)
{
    ESP_LOGI(TAG, "\"%s\": {\"mode\": %s}", typ_str, name_pump_mode(msg->mode));
}

void
_pump_state(char const * const typ_str, mPumpState_a5_t const * const msg)
{
    if (msg->state == 0x04 || msg->state == 0x0A) {
        ESP_LOGI(TAG, "\"%s\": {\"running\": %u}", typ_str, msg->state == 0x0A);
    }
}

void
_wildchard_req(char const * const typ_str)
{
    ESP_LOGI(TAG, "\"%s\": {}", typ_str);
}

void
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
        if (pump_state->running) {
            cJSON_AddTrueToObject(root, "running");
        } else {
            cJSON_AddFalseToObject(root, "running");
        }
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
        char * const buf = cJSON_Print(root);
        cJSON_Delete(root);
        ESP_LOGI(TAG, "%s", buf);
        free(buf);
    }
}

void
_ctrl_set_ack(char const * const typ_str, mCtrlSetAck_a5_t const * const msg)
{
    ESP_LOGI(TAG, "\"%s\": {\"ack\": \"%s\"}", typ_str, name_datalink_a5_ctrl_msgtype(msg->typ));
}

bool
poolstate_receive_update(network_msg_t const * const msg, poolstate_t * const state)
{
    char const * const typ_str = name_network_msgtyp(msg->typ);

    poolstate_t old_state;
    memcpy(&old_state, state, sizeof(poolstate_t));

    switch(msg->typ) {
        case NETWORK_MSGTYP_PUMP_REG_SET:
            _pump_reg_set(typ_str, msg->u.pump_reg_set);
            break;
        case NETWORK_MSGTYP_PUMP_REG_SET_RESP:
            _pump_reg_set_resp(typ_str, msg->u.pump_reg_set_resp);
            break;
        case NETWORK_MSGTYP_PUMP_CTRL:
            _pump_ctrl(typ_str, msg->u.pump_ctrl);
            break;
        case NETWORK_MSGTYP_PUMP_MODE:
            _pump_mode(typ_str, msg->u.pump_mode);
            break;
        case NETWORK_MSGTYP_PUMP_STATE:
            _pump_state(typ_str, msg->u.pump_state);
            break;
        case NETWORK_MSGTYP_PUMP_STATUS_REQ:
            _wildchard_req(typ_str);
             break;
        case NETWORK_MSGTYP_PUMP_STATUS:
            _pump_status(typ_str, msg->u.pump_status, &state->pump);
            break;
        case NETWORK_MSGTYP_CTRL_SET_ACK:
            _ctrl_set_ack(typ_str, msg->u.ctrl_set_ack);
            break;
        case NETWORK_MSGTYP_CTRL_CIRCUIT_SET:
        case NETWORK_MSGTYP_CTRL_SCHED_REQ:
        case NETWORK_MSGTYP_CTRL_SCHED:
        case NETWORK_MSGTYP_CTRL_STATE_REQ:
        case NETWORK_MSGTYP_CTRL_STATE:
        case NETWORK_MSGTYP_CTRL_STATE_SET:
        case NETWORK_MSGTYP_CTRL_TIME_REQ:
        case NETWORK_MSGTYP_CTRL_TIME:
        case NETWORK_MSGTYP_CTRL_TIME_SET:
        case NETWORK_MSGTYP_CTRL_HEAT_REQ:
        case NETWORK_MSGTYP_CTRL_HEAT:
        case NETWORK_MSGTYP_CTRL_HEAT_SET:
        case NETWORK_MSGTYP_CTRL_LAYOUT_REQ:
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
    return memcmp(&old_state, state, sizeof(poolstate_t)) != 0;
}
