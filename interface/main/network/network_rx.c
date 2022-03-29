/**
 * @brief Network layer: decode datalink_pkt to network_msg
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2022, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>

#include "../datalink/datalink.h"
#include "../datalink/datalink_pkt.h"
#include "../utils/utils.h"
#include "network.h"

static char const * const TAG = "network_rx";

typedef struct hdr_data_hdr_copy_t {
    uint8_t dst;  // destination
    uint8_t src;  // source
    uint8_t typ;  // message type
} hdr_data_hdr_copy_t;

/*
 *
 */

static void
_decode_msg_a5_ctrl(datalink_pkt_t const * const pkt, network_msg_t * const network)
{
    network->typ = MSG_TYP_NONE;

    switch (pkt->prot_typ) {

        case NETWORK_TYP_CTRL_SET_ACK:
            if (pkt->data_len == sizeof(network_msg_ctrl_set_ack_t)) {
                network->typ = MSG_TYP_CTRL_SET_ACK;
                network->u.ctrl_set_ack = (network_msg_ctrl_set_ack_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_CIRCUIT_SET:
            if (pkt->data_len == sizeof(network_msg_ctrl_circuit_set_t)) {
                network->typ = MSG_TYP_CTRL_CIRCUIT_SET;
                network->u.ctrl_circuit_set = (network_msg_ctrl_circuit_set_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_SCHED_REQ:
            if (pkt->data_len == 0) {
                network->typ = MSG_TYP_CTRL_SCHED_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_SCHED_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_sched_resp_t)) {
                network->typ = MSG_TYP_CTRL_SCHED_RESP;
                network->u.ctrl_sched_resp = (network_msg_ctrl_sched_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_STATE_BCAST:
            if (pkt->data_len == sizeof(network_msg_ctrl_state_bcast_t)) {
                network->typ = MSG_TYP_CTRL_STATE_BCAST;
                network->u.ctrl_state = (network_msg_ctrl_state_bcast_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_TIME_REQ:
            if (pkt->data_len == 0) {
                network->typ = MSG_TYP_CTRL_TIME_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_TIME_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_time_resp_t)) {
                network->typ = MSG_TYP_CTRL_TIME_RESP;
                network->u.ctrl_time_resp = (network_msg_ctrl_time_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_TIME_SET:
            if (pkt->data_len == sizeof(network_msg_ctrl_time_set_t)) {
                network->typ = MSG_TYP_CTRL_TIME_SET;
                network->u.ctrl_time_set = (network_msg_ctrl_time_set_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT_REQ:
            if (pkt->data_len == 0) {
                network->typ = MSG_TYP_CTRL_HEAT_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_heat_resp_t)) {
                network->typ = MSG_TYP_CTRL_HEAT_RESP;
                network->u.ctrl_heat_resp = (network_msg_ctrl_heat_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT_SET:
            if (pkt->data_len == sizeof(network_msg_ctrl_heat_set_t)) {
                network->typ = MSG_TYP_CTRL_HEAT_SET;
                network->u.ctrl_heat_set = (network_msg_ctrl_heat_set_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_LAYOUT_REQ:
            if (pkt->data_len == 0) {
                network->typ = MSG_TYP_CTRL_LAYOUT_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_LAYOUT_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_layout_resp_t)) {
                network->typ = MSG_TYP_CTRL_LAYOUT;
                network->u.ctrl_layout_resp = (network_msg_ctrl_layout_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_LAYOUT_SET:
            if (pkt->data_len == sizeof(network_msg_ctrl_layout_set_t)) {
                network->typ = MSG_TYP_CTRL_LAYOUT_SET;
                network->u.ctrl_layout_set = (network_msg_ctrl_layout_set_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_VERSION_REQ:
            if (pkt->data_len == sizeof(network_msg_ctrl_version_req_t)) {
                network->typ = MSG_TYP_CTRL_VERSION_REQ;
                network->u.ctrl_version_req = (network_msg_ctrl_version_req_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_VERSION_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_version_resp_t)) {
                network->typ = MSG_TYP_CTRL_VERSION_RESP;
                network->u.ctrl_version_resp = (network_msg_ctrl_version_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_VALVE_REQ:
            if (pkt->data_len == sizeof(network_msg_ctrl_valve_req_t)) {
                network->typ = MSG_TYP_CTRL_VALVE_REQ;
                network->u.ctrl_valve_req = (network_msg_ctrl_valve_req_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_VALVE_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_valve_resp_t)) {
                network->typ = MSG_TYP_CTRL_VALVE_RESP;
                network->u.ctrl_valve_resp = (network_msg_ctrl_valve_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_SOLARPUMP_REQ:
            if (pkt->data_len == sizeof(network_msg_ctrl_solarpump_req_t)) {
                network->typ = MSG_TYP_CTRL_SOLARPUMP_REQ;
                network->u.ctrl_solarpump_req = (network_msg_ctrl_solarpump_req_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_SOLARPUMP_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_solarpump_resp_t)) {
                network->typ = MSG_TYP_CTRL_SOLARPUMP_RESP;
                network->u.ctrl_solarpump_resp = (network_msg_ctrl_solarpump_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_DELAY_REQ:
            if (pkt->data_len == sizeof(network_msg_ctrl_delay_req_t)) {
                network->typ = MSG_TYP_CTRL_DELAY_REQ;
                network->u.ctrl_delay_req = (network_msg_ctrl_delay_req_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_DELAY_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_delay_resp_t)) {
                network->typ = MSG_TYP_CTRL_DELAY_RESP;
                network->u.ctrl_delay_resp = (network_msg_ctrl_delay_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT_SETPT_REQ:
            if (pkt->data_len == sizeof(network_msg_ctrl_heat_setpt_req_t)) {
                network->typ = MSG_TYP_CTRL_HEAT_SETPT_REQ;
                network->u.ctrl_heat_set_req = (network_msg_ctrl_heat_setpt_req_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT_SETPT_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_heat_setpt_resp_t)) {
                network->typ = MSG_TYP_CTRL_HEAT_SETPT_RESP;
                network->u.ctrl_heat_set_resp = (network_msg_ctrl_heat_setpt_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_SCHEDS_REQ:
            if (pkt->data_len == sizeof(network_msg_ctrl_scheds_req_t)) {
                network->typ = MSG_TYP_CTRL_SCHEDS_REQ;
                network->u.ctrl_scheds_req = (network_msg_ctrl_scheds_req_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_SCHEDS_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_scheds_resp_t)) {
                network->typ = MSG_TYP_CTRL_SCHEDS_RESP;
                network->u.ctrl_scheds_resp = (network_msg_ctrl_scheds_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_CIRC_NAMES_REQ:
            if (pkt->data_len == sizeof(network_msg_ctrl_circ_names_req_t)) {
                network->typ = MSG_TYP_CTRL_CIRC_NAMES_REQ;
                network->u.ctrl_circ_names_req = (network_msg_ctrl_circ_names_req_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_CIRC_NAMES_RESP:
            if (pkt->data_len == sizeof(network_msg_ctrl_circ_names_resp_t)) {
                network->typ = MSG_TYP_CTRL_CIRC_NAMES_RESP;
                network->u.ctrl_circ_names_resp = (network_msg_ctrl_circ_names_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_CHEM_REQ:
            if (pkt->data_len == sizeof(network_msg_ctrl_chem_req_t)) {
                network->typ = MSG_TYP_CTRL_CHEM_REQ;
                network->u.ctrl_chem_req = (network_msg_ctrl_chem_req_t *) pkt->data;
            }
            break;
        default:
            if (CONFIG_POOL_DBGLVL_NETWORK >0) {
                ESP_LOGW(TAG, "unknown A5_CTRL pkt->prot_typ (0x%02X)", pkt->prot_typ);
            }
            break;
    }
};

/*
 *
 */

static void
_decode_msg_a5_pump(datalink_pkt_t const * const pkt, network_msg_t * const network)
{
    network->typ = MSG_TYP_NONE;
	bool toPump = (datalink_groupaddr(pkt->dst) == DATALINK_ADDRGROUP_PUMP);

    switch (pkt->prot_typ) {
        case NETWORK_TYP_PUMP_REG:
            if (toPump) {
                if (pkt->data_len == sizeof(network_msg_pump_reg_set_t)) {
                    network->typ = MSG_TYP_PUMP_REG_SET;
                    network->u.pump_reg_set = (network_msg_pump_reg_set_t *) pkt->data;
                }
            } else {
                if (pkt->data_len == sizeof(network_msg_pump_reg_resp_t)) {
                    network->typ = MSG_TYP_PUMP_REG_RESP;
                    network->u.pump_reg_set_resp = (network_msg_pump_reg_resp_t *) pkt->data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_CTRL:
            if (toPump) {
                if (pkt->data_len == sizeof(network_msg_pump_ctrl_t)) {
                    network->typ = MSG_TYP_PUMP_CTRL_SET;
                    network->u.pump_ctrl = (network_msg_pump_ctrl_t *) pkt->data;
                }
            } else {
                if (pkt->data_len == sizeof(network_msg_pump_ctrl_t)) {
                    network->typ = MSG_TYP_PUMP_CTRL_RESP;
                    network->u.pump_ctrl = (network_msg_pump_ctrl_t *) pkt->data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_MODE:
            if (toPump) {
                if (pkt->data_len == sizeof(network_msg_pump_mode_t)) {
                    network->typ = MSG_TYP_PUMP_MODE_SET;
                    network->u.pump_mode = (network_msg_pump_mode_t *) pkt->data;
                }
            } else {
                if (pkt->data_len == sizeof(network_msg_pump_mode_t)) {
                    network->typ = MSG_TYP_PUMP_MODE_RESP;
                    network->u.pump_mode = (network_msg_pump_mode_t *) pkt->data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_RUN:
            if (toPump) {
                if (pkt->data_len == sizeof(network_msg_pump_run_t)) {
                    network->typ = MSG_TYP_PUMP_RUN_SET;
                    network->u.pump_run = (network_msg_pump_run_t *) pkt->data;
                }
            } else {
                if (pkt->data_len == sizeof(network_msg_pump_run_t)) {
                    network->typ = MSG_TYP_PUMP_RUN_RESP;
                    network->u.pump_run = (network_msg_pump_run_t *) pkt->data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_STATUS:
            if (toPump) {
                if (pkt->data_len == 0) {
                    network->typ = MSG_TYP_PUMP_STATUS_REQ;
                }
            } else {
                if (pkt->data_len == sizeof(network_msg_pump_status_resp_t)) {
                    network->typ = MSG_TYP_PUMP_STATUS_RESP;
                    network->u.pump_status_resp = (network_msg_pump_status_resp_t *) pkt->data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_FF:
            // silently ignore
            break;
        default:
            if (CONFIG_POOL_DBGLVL_NETWORK >0) {
                ESP_LOGW(TAG, "unknown A5 pump typ %u", pkt->prot_typ);
            }
            break;
    }
}

/*
 *
 */

static void
_decode_msg_ic_chlor(datalink_pkt_t const * const pkt, network_msg_t * const network)
{
    network->typ = MSG_TYP_NONE;

    switch (pkt->prot_typ) {
        case NETWORK_TYP_CHLOR_PING_REQ:
            if (pkt->data_len == sizeof(network_msg_chlor_ping_req_t)) {
                network->typ = MSG_TYP_CHLOR_PING_REQ;
                network->u.chlor_ping_req = (network_msg_chlor_ping_req_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CHLOR_PING_RESP:
            if (pkt->data_len == sizeof(network_msg_chlor_ping_resp_t)) {
                network->typ = MSG_TYP_CHLOR_PING_RESP;
                network->u.chlor_ping = (network_msg_chlor_ping_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CHLOR_NAME_RESP:
            if (pkt->data_len == sizeof(network_msg_chlor_name_resp_t)) {
                network->typ = MSG_TYP_CHLOR_NAME_RESP;
                network->u.chlor_name_resp = (network_msg_chlor_name_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CHLOR_LEVEL_SET:
            if (pkt->data_len == sizeof(network_msg_chlor_level_set_t)) {
                network->typ = MSG_TYP_CHLOR_LEVEL_SET;
                network->u.chlor_level_set = (network_msg_chlor_level_set_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CHLOR_LEVEL_RESP:
            if (pkt->data_len == sizeof(network_msg_chlor_level_resp_t)) {
                network->typ = MSG_TYP_CHLOR_LEVEL_RESP;
                network->u.chlor_level_resp = (network_msg_chlor_level_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CHLOR_NAME_REQ:
            if (pkt->data_len == sizeof(network_msg_chlor_name_req_t)) {
                network->typ = MSG_TYP_CHLOR_NAME_REQ;
                network->u.chlor_name_req = (network_msg_chlor_name_req_t *) pkt->data;
            }
            break;
        default:
            if (CONFIG_POOL_DBGLVL_NETWORK >0) {
                ESP_LOGW(TAG, "unknown IC typ %u", pkt->prot_typ);
            }
            break;
    }
}

/*
 * 
 */

esp_err_t
network_rx_msg(datalink_pkt_t const * const pkt, network_msg_t * const msg, bool * const txOpportunity)
{
    // reset mechanism that converts various formats to string
	name_reset_idx();

#if 1
    // silently ignore packets that we can't decode
    datalink_addrgroup_t const dst = datalink_groupaddr(pkt->dst);
    if ((pkt->prot == DATALINK_PROT_A5_CTRL && dst == DATALINK_ADDRGROUP_X09) ||
        (pkt->prot == DATALINK_PROT_IC && dst != DATALINK_ADDRGROUP_ALL && dst != DATALINK_ADDRGROUP_CHLOR)) {
        return ESP_FAIL;
    }
#endif
	switch (pkt->prot) {
		case DATALINK_PROT_A5_CTRL:
            _decode_msg_a5_ctrl(pkt, msg);
			break;
		case DATALINK_PROT_A5_PUMP:
            _decode_msg_a5_pump(pkt, msg);
			break;
		case DATALINK_PROT_IC:
            _decode_msg_ic_chlor(pkt, msg);
			break;
        default:
            if (CONFIG_POOL_DBGLVL_NETWORK >0) {
                ESP_LOGW(TAG, "unknown prot %u", pkt->prot);
            }
	}
    *txOpportunity =
        pkt->prot == DATALINK_PROT_A5_CTRL &&
        datalink_groupaddr(pkt->src) == DATALINK_ADDRGROUP_CTRL &&
        datalink_groupaddr(pkt->dst) == DATALINK_ADDRGROUP_ALL;

    return msg->typ == MSG_TYP_NONE ? ESP_FAIL : ESP_OK;
}
