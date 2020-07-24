/**
 * @brief Decode network messages received over the rs-485 bus
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>

#include "../datalink/datalink.h"
#include "../utils/utils_str.h"
#include "network.h"

static char const * const TAG = "receive_msg";

static void
_decodeA5_ctrl(datalink_pkt_t const * const datalink, network_msg_t * const network)
{
#if 0
    uint len = 0;
    uint buf_size = 80;
    char buf[buf_size];

    for (uint ii = 0; ii < datalink->hdr.len; ii++) {
        len += snprintf(buf + len, buf_size - len, " %02X", datalink->data[ii]);
    }
    ESP_LOGI(TAG, "%s (data)", buf);
#endif
    network->typ = NETWORK_MSGTYP_NONE;

    switch ((datalink_ctrl_typ_t) datalink->hdr.typ) {
        // response to various set requests
        case DATALINK_CTRL_TYP_SET_ACK:
            if (datalink->hdr.len == sizeof(mCtrlSetAck_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_SET_ACK;
                network->u.ctrl_set_ack = (mCtrlSetAck_a5_t *) datalink->data;
            }
            break;
        // set circuit request (there appears to be no separate "get circuit request")
        case DATALINK_CTRL_TYP_CIRCUIT_SET:
            if (datalink->hdr.len == sizeof(mCtrlCircuitSet_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_CIRCUIT_SET;
                network->u.ctrl_circuit_set = (mCtrlCircuitSet_a5_t *) datalink->data;
            }
            break;
            // various get requests
        case DATALINK_CTRL_TYP_TIME_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSGTYP_CTRL_TIME_REQ;
            }
            break;
        case DATALINK_CTRL_TYP_STATE_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSGTYP_CTRL_STATE_REQ;
            }
            break;
        case DATALINK_CTRL_TYP_HEAT_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSGTYP_CTRL_HEAT_REQ;
            }
            break;
        case DATALINK_CTRL_TYP_SCHED_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSGTYP_CTRL_SCHED_REQ;
            }
            break;
        case DATALINK_CTRL_TYP_LAYOUT_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSGTYP_CTRL_LAYOUT_REQ;
            }
            break;
            // schedule: get response / set request
        case DATALINK_CTRL_TYP_SCHED:
            if (datalink->hdr.len == sizeof(mCtrlSched_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_SCHED;
                network->u.ctrl_sched = (mCtrlSched_a5_t *) datalink->data;
            }
            break;
            // state: get response / set request
        case DATALINK_CTRL_TYP_STATE:
            if (datalink->hdr.len == sizeof(mCtrlState_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_STATE;
                network->u.ctrl_state = (mCtrlState_a5_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_STATE_SET:
            // unfinished
            //if (datalink->hdr.len == sizeof(mCtrlState_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_STATE_SET;
                //network->u.ctrl_state = (mCtrlState_a5_t *) datalink->data;
            //}
            break;
            // time: get response / set request
        case DATALINK_CTRL_TYP_TIME:
            if (datalink->hdr.len == sizeof(mCtrlTime_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_TIME;
                network->u.ctrl_time = (mCtrlTime_a5_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_TIME_SET:
            if (datalink->hdr.len == sizeof(mCtrlTimeSet_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_TIME_SET;
                network->u.ctrl_time_set = (mCtrlTimeSet_a5_t *) datalink->data;
            }
            break;
            // heat: get response / set request
        case DATALINK_CTRL_TYP_HEAT:
            if (datalink->hdr.len == sizeof(mCtrlHeat_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_HEAT;
                network->u.ctrl_heat = (mCtrlHeat_a5_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_HEAT_SET:
            if (datalink->hdr.len == sizeof(mCtrlHeatSet_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_HEAT_SET;
                network->u.ctrl_heat_set = (mCtrlHeatSet_a5_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_LAYOUT:
            if (datalink->hdr.len == sizeof(mCtrlLayout_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_LAYOUT;
                network->u.ctrl_layout = (mCtrlLayout_a5_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_LAYOUT_SET:
            if (datalink->hdr.len == sizeof(mCtrlLayoutSet_a5_t)) {
                network->typ = NETWORK_MSGTYP_CTRL_LAYOUT_SET;
                network->u.ctrl_layout_set = (mCtrlLayoutSet_a5_t *) datalink->data;
            }
            break;
        default:
            ESP_LOGW(TAG, "unknown A5 ctrl typ %u", datalink->hdr.typ);
            break;
    }
};

static void
_decodeA5_pump(datalink_pkt_t const * const datalink, network_msg_t * const network)
{
    network->typ = NETWORK_MSGTYP_NONE;
	bool toPump = (network_group_addr(datalink->hdr.dst) == NETWORK_ADDRGROUP_PUMP);

    switch ((datalink_pump_typ_t)datalink->hdr.typ) {
        case DATALINK_PUMP_TYP_REGULATE:
            if (toPump) {
                if (datalink->hdr.len == sizeof(mPumpRegulateSet_a5_t)) {
                    network->typ = NETWORK_MSGTYP_PUMP_REG_SET;
                    network->u.pump_reg_set = (mPumpRegulateSet_a5_t *) datalink->data;
                }
            } else {
                if (datalink->hdr.len == sizeof(mPumpRegulateSetResp_a5_t)) {
                    network->typ = NETWORK_MSGTYP_PUMP_REG_SET_RESP;
                    network->u.pump_reg_set_resp = (mPumpRegulateSetResp_a5_t *) datalink->data;
                }
            }
            break;
        case DATALINK_PUMP_TYP_CTRL:
            if (datalink->hdr.len == sizeof(mPumpCtrl_a5_t)) {
                network->typ = NETWORK_MSGTYP_PUMP_CTRL;
                network->u.pump_ctrl = (mPumpCtrl_a5_t *) datalink->data;
            }
            break;
        case DATALINK_PUMP_TYP_MODE:
            if (datalink->hdr.len == sizeof(mPumpMode_a5_t)) {
                network->typ = NETWORK_MSGTYP_PUMP_MODE;
                network->u.pump_mode = (mPumpMode_a5_t *) datalink->data;
            }
            break;
        case DATALINK_PUMP_TYP_STATE:
            if (datalink->hdr.len == sizeof(mPumpRunning_a5_t)) {
                network->typ = NETWORK_MSGTYP_PUMP_RUNNING;
                network->u.pump_running = (mPumpRunning_a5_t *) datalink->data;
            }
            break;
        case DATALINK_PUMP_TYP_STATUS:
            if (toPump) {
                if (datalink->hdr.len == 0) {
                    network->typ = NETWORK_MSGTYP_PUMP_STATUS_REQ;
                }
            } else {
                if (datalink->hdr.len == sizeof(mPumpStatus_a5_t)) {
                    network->typ = NETWORK_MSGTYP_PUMP_STATUS;
                    network->u.pump_status = (mPumpStatus_a5_t *) datalink->data;
                }
            }
            break;
        case DATALINK_PUMP_TYP_0xFF:
            // silently ignore
            break;
        default:
            ESP_LOGW(TAG, "unknown A5 pump typ %u", datalink->hdr.typ);
            break;
    }
}

static void
_decodeIC_chlor(datalink_pkt_t const * const datalink, network_msg_t * const network)
{
    network->typ = NETWORK_MSGTYP_NONE;

    switch ((datalink_chlor_typ_t) datalink->hdr.typ) {
        case DATALINK_CHLOR_TYP_PING_REQ:
            if (datalink->hdr.len == sizeof(mChlorPingReq_ic_t)) {
                network->typ = NETWORK_MSGTYP_CHLOR_PING_REQ;
                network->u.chlor_ping_req = (mChlorPingReq_ic_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_PING:
            if (datalink->hdr.len == sizeof(mChlorPing_ic_t)) {
                network->typ = NETWORK_MSGTYP_CHLOR_PING;
                network->u.chlor_ping = (mChlorPing_ic_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_NAME:
            if (datalink->hdr.len == sizeof(mChlorName_ic_t)) {
                network->typ = NETWORK_MSGTYP_CHLOR_NAME;
                network->u.chlor_name = (mChlorName_ic_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_LEVEL_SET:
            if (datalink->hdr.len == sizeof(mChlorLevelSet_ic_t)) {
                network->typ = NETWORK_MSGTYP_CHLOR_LEVEL_SET;
                network->u.chlor_level_set = (mChlorLevelSet_ic_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_LEVEL_RESP:
            if (datalink->hdr.len == sizeof(mChlorLevelResp_ic_t)) {
                network->typ = NETWORK_MSGTYP_CHLOR_LEVEL_RESP;
                network->u.chlor_level_resp = (mChlorLevelResp_ic_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_X14:
            // silently ignore
            break;
        default:
            ESP_LOGW(TAG, "unknown IC typ %u", datalink->hdr.typ);
            break;
    }
}

bool
network_receive_msg(datalink_pkt_t const * const datalink, network_msg_t * const network, bool * const txOpportunity)
{
	NETWORK_ADDRGROUP_t src = network_group_addr(datalink->hdr.src);
	NETWORK_ADDRGROUP_t dst = network_group_addr(datalink->hdr.dst);

	if (dst == NETWORK_ADDRGROUP_UNUSED9) {
		return false; // stick our head in the sand
	}

	name_reset_idx();

	switch (datalink->proto) {
		case DATALINK_PROT_A5:
			if (src == NETWORK_ADDRGROUP_PUMP || dst == NETWORK_ADDRGROUP_PUMP) {
                _decodeA5_pump(datalink, network);
			} else {
                _decodeA5_ctrl(datalink, network);
			}
			break;
		case DATALINK_PROT_IC:
			if (dst == NETWORK_ADDRGROUP_ALL || dst == NETWORK_ADDRGROUP_CHLOR) {
                _decodeIC_chlor(datalink, network);
			}
			break;
        default:
            ESP_LOGW(TAG, "unknown proto %u", datalink->proto);

	}
    *txOpportunity =
        datalink->proto == DATALINK_PROT_A5 &&
        network_group_addr(datalink->hdr.src) == NETWORK_ADDRGROUP_CTRL &&
        network_group_addr(datalink->hdr.dst) == NETWORK_ADDRGROUP_ALL;

    return network->typ != NETWORK_MSGTYP_NONE;
}
