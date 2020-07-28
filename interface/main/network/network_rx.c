/**
 * @brief Data Link layer: decode packets from the data link layer to messages
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>

#include "../datalink/datalink.h"
#include "../utils/utils.h"
#include "../poolstate/msg.h"
#include "network.h"

static char const * const TAG = "network_rx";

static void
_decode_msg_a5_ctrl(datalink_pkt_t const * const pkt, network_msg_t * const network)
{
    network->typ = MSG_TYP_NONE;
    network_typ_ctrl_t hdr_typ = (network_typ_ctrl_t) pkt->head->a5.hdr.typ;

    switch (hdr_typ) {

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
        case NETWORK_TYP_CTRL_SCHED:
            if (pkt->data_len == sizeof(network_msg_ctrl_sched_resp_t)) {
                network->typ = MSG_TYP_CTRL_SCHED_RESP;
                network->u.ctrl_sched_resp = (network_msg_ctrl_sched_resp_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_STATE_REQ:
            if (pkt->data_len == 0) {
                network->typ = MSG_TYP_CTRL_STATE_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_STATE:
            if (pkt->data_len == sizeof(network_msg_ctrl_state_t)) {
                network->typ = MSG_TYP_CTRL_STATE;
                network->u.ctrl_state = (network_msg_ctrl_state_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_STATE_SET:
            // unfinished
            if (pkt->data_len == sizeof(network_msg_ctrl_state_set_t)) {
                network->typ = MSG_TYP_CTRL_STATE_SET;
                network->u.ctrl_state = (network_msg_ctrl_state_set_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_TIME_REQ:
            if (pkt->data_len == 0) {
                network->typ = MSG_TYP_CTRL_TIME_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_TIME:
            if (pkt->data_len == sizeof(network_msg_ctrl_time_t)) {
                network->typ = MSG_TYP_CTRL_TIME;
                network->u.ctrl_time = (network_msg_ctrl_time_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_TIME_SET:
            if (pkt->data_len == sizeof(network_msg_ctrl_time_t)) {
                network->typ = MSG_TYP_CTRL_TIME_SET;
                network->u.ctrl_time = (network_msg_ctrl_time_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT_REQ:
            if (pkt->data_len == 0) {
                network->typ = MSG_TYP_CTRL_HEAT_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT:
            if (pkt->data_len == sizeof(network_msg_ctrl_heat_t)) {
                network->typ = MSG_TYP_CTRL_HEAT;
                network->u.ctrl_heat = (network_msg_ctrl_heat_t *) pkt->data;
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
        case NETWORK_TYP_CTRL_LAYOUT:
            if (pkt->data_len == sizeof(network_msg_ctrl_layout_t)) {
                network->typ = MSG_TYP_CTRL_LAYOUT;
                network->u.ctrl_layout = (network_msg_ctrl_layout_t *) pkt->data;
            }
            break;
        case NETWORK_TYP_CTRL_LAYOUT_SET:
            if (pkt->data_len == sizeof(network_msg_ctrl_layout_set_t)) {
                network->typ = MSG_TYP_CTRL_LAYOUT_SET;
                network->u.ctrl_layout_set = (network_msg_ctrl_layout_set_t *) pkt->data;
            }
            break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR) {
                ESP_LOGW(TAG, "unknown A5 ctrl typ (0x%02X)", hdr_typ);
            }
            break;
    }
};

static void
_decode_msg_a5_pump(datalink_pkt_t const * const pkt, network_msg_t * const network)
{
    network->typ = MSG_TYP_NONE;
	bool toPump = (datalink_groupaddr(pkt->head->a5.hdr.dst) == DATALINK_ADDRGROUP_PUMP);
    network_typ_pump_t hdr_typ = (network_typ_pump_t)  pkt->head->a5.hdr.typ;

    switch (hdr_typ) {
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
        case NETWORK_TYP_PUMP_0xFF:
            // silently ignore
            break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR) {
                ESP_LOGW(TAG, "unknown A5 pump typ %u", hdr_typ);
            }
            break;
    }
}

static void
_decode_msg_ic_chlor(datalink_pkt_t const * const pkt, network_msg_t * const network)
{
    network->typ = MSG_TYP_NONE;
    network_typ_chlor_t hdr_typ = (network_typ_chlor_t)  pkt->head->ic.hdr.typ;

    switch (hdr_typ) {
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
        case NETWORK_TYP_CHLOR_NAME:
            if (pkt->data_len == sizeof(network_msg_chlor_name_t)) {
                network->typ = MSG_TYP_CHLOR_NAME;
                network->u.chlor_name = (network_msg_chlor_name_t *) pkt->data;
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
        case NETWORK_TYP_CHLOR_X14:
            // silently ignore
            break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR) {
                ESP_LOGW(TAG, "unknown IC typ %u", hdr_typ);
            }
            break;
    }
}

bool
network_rx_msg(datalink_pkt_t const * const pkt, network_msg_t * const msg, bool * const txOpportunity)
{
	name_reset_idx();

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
            if (CONFIG_POOL_DBG_NETWORK_ONERROR) {
                ESP_LOGW(TAG, "unknown proto %u", pkt->prot);
            }
	}
    *txOpportunity =
        pkt->prot == DATALINK_PROT_A5_CTRL &&
        datalink_groupaddr(pkt->head->a5.hdr.src) == DATALINK_ADDRGROUP_CTRL &&
        datalink_groupaddr(pkt->head->a5.hdr.dst) == DATALINK_ADDRGROUP_ALL;

    return msg->typ != MSG_TYP_NONE;
}
