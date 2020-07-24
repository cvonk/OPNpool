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
#include "network.h"

static char const * const TAG = "network_rx";

static void
_decode_msg_a5_ctrl(datalink_pkt_t const * const datalink, network_msg_t * const network)
{
    network->typ = NETWORK_MSG_TYP_NONE;

    switch ((datalink_ctrl_typ_t) datalink->hdr.typ) {

        case DATALINK_CTRL_TYP_SET_ACK:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_set_ack_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_SET_ACK;
                network->u.ctrl_set_ack = (network_msg_ctrl_set_ack_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_CIRCUIT_SET:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_circuit_set_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_CIRCUIT_SET;
                network->u.ctrl_circuit_set = (network_msg_ctrl_circuit_set_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_TIME_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSG_TYP_CTRL_TIME_REQ;
            }
            break;
        case DATALINK_CTRL_TYP_STATE_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSG_TYP_CTRL_STATE_REQ;
            }
            break;
        case DATALINK_CTRL_TYP_HEAT_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSG_TYP_CTRL_HEAT_REQ;
            }
            break;
        case DATALINK_CTRL_TYP_SCHED_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSG_TYP_CTRL_SCHED_REQ;
            }
            break;
        case DATALINK_CTRL_TYP_LAYOUT_REQ:
            if (datalink->hdr.len == 0) {
                network->typ = NETWORK_MSG_TYP_CTRL_LAYOUT_REQ;
            }
            break;
        case DATALINK_CTRL_TYP_SCHED:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_sched_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_SCHED;
                network->u.ctrl_sched = (network_msg_ctrl_sched_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_STATE:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_state_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_STATE;
                network->u.ctrl_state = (network_msg_ctrl_state_t *) datalink->data;
            }
            break;

        case DATALINK_CTRL_TYP_STATE_SET:
            // unfinished
            //if (datalink->hdr.len == sizeof(network_msg_ctrl_state_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_STATE_SET;
                //network->u.ctrl_state = (network_msg_ctrl_state_t *) datalink->data;
            //}
            break;
        case DATALINK_CTRL_TYP_TIME:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_time_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_TIME;
                network->u.ctrl_time = (network_msg_ctrl_time_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_TIME_SET:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_time_set_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_TIME_SET;
                network->u.ctrl_time_set = (network_msg_ctrl_time_set_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_HEAT:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_heat_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_HEAT;
                network->u.ctrl_heat = (network_msg_ctrl_heat_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_HEAT_SET:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_heat_set_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_HEAT_SET;
                network->u.ctrl_heat_set = (network_msg_ctrl_heat_set_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_LAYOUT:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_layout_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_LAYOUT;
                network->u.ctrl_layout = (network_msg_ctrl_layout_t *) datalink->data;
            }
            break;
        case DATALINK_CTRL_TYP_LAYOUT_SET:
            if (datalink->hdr.len == sizeof(network_msg_ctrl_layout_set_t)) {
                network->typ = NETWORK_MSG_TYP_CTRL_LAYOUT_SET;
                network->u.ctrl_layout_set = (network_msg_ctrl_layout_set_t *) datalink->data;
            }
            break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR == 'y') {
                ESP_LOGW(TAG, "unknown A5 ctrl typ %u", datalink->hdr.typ);
            }
            break;
    }
};

static void
_decode_msg_a5_pump(datalink_pkt_t const * const datalink, network_msg_t * const network)
{
    network->typ = NETWORK_MSG_TYP_NONE;
	bool toPump = (network_groupaddr(datalink->hdr.dst) == NETWORK_ADDRGROUP_PUMP);

    switch ((datalink_pump_typ_t)datalink->hdr.typ) {
        case DATALINK_PUMP_TYP_REGULATE:
            if (toPump) {
                if (datalink->hdr.len == sizeof(network_msg_pump_reg_set_t)) {
                    network->typ = NETWORK_MSG_TYP_PUMP_REG_SET;
                    network->u.pump_reg_set = (network_msg_pump_reg_set_t *) datalink->data;
                }
            } else {
                if (datalink->hdr.len == sizeof(network_msg_pump_reg_resp_t)) {
                    network->typ = NETWORK_MSG_TYP_PUMP_REG_SET_RESP;
                    network->u.pump_reg_set_resp = (network_msg_pump_reg_resp_t *) datalink->data;
                }
            }
            break;
        case DATALINK_PUMP_TYP_CTRL:
            if (datalink->hdr.len == sizeof(network_msg_pump_ctrl_t)) {
                network->typ = NETWORK_MSG_TYP_PUMP_CTRL;
                network->u.pump_ctrl = (network_msg_pump_ctrl_t *) datalink->data;
            }
            break;
        case DATALINK_PUMP_TYP_MODE:
            if (datalink->hdr.len == sizeof(network_msg_pump_mode_t)) {
                network->typ = NETWORK_MSG_TYP_PUMP_MODE;
                network->u.pump_mode = (network_msg_pump_mode_t *) datalink->data;
            }
            break;
        case DATALINK_PUMP_TYP_STATE:
            if (datalink->hdr.len == sizeof(network_msg_pump_running_t)) {
                network->typ = NETWORK_MSG_TYP_PUMP_RUNNING;
                network->u.pump_running = (network_msg_pump_running_t *) datalink->data;
            }
            break;
        case DATALINK_PUMP_TYP_STATUS:
            if (toPump) {
                if (datalink->hdr.len == 0) {
                    network->typ = NETWORK_MSG_TYP_PUMP_STATUS_REQ;
                }
            } else {
                if (datalink->hdr.len == sizeof(network_msg_pump_status_t)) {
                    network->typ = NETWORK_MSG_TYP_PUMP_STATUS;
                    network->u.pump_status = (network_msg_pump_status_t *) datalink->data;
                }
            }
            break;
        case DATALINK_PUMP_TYP_0xFF:
            // silently ignore
            break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR == 'y') {
                ESP_LOGW(TAG, "unknown A5 pump typ %u", datalink->hdr.typ);
            }
            break;
    }
}

static void
_decode_msg_ic_chlor(datalink_pkt_t const * const datalink, network_msg_t * const network)
{
    network->typ = NETWORK_MSG_TYP_NONE;

    switch ((datalink_chlor_typ_t) datalink->hdr.typ) {
        case DATALINK_CHLOR_TYP_PING_REQ:
            if (datalink->hdr.len == sizeof(network_msg_chlor_ping_req_t)) {
                network->typ = NETWORK_MSG_TYP_CHLOR_PING_REQ;
                network->u.chlor_ping_req = (network_msg_chlor_ping_req_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_PING:
            if (datalink->hdr.len == sizeof(network_msg_chlor_ping_t)) {
                network->typ = NETWORK_MSG_TYP_CHLOR_PING;
                network->u.chlor_ping = (network_msg_chlor_ping_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_NAME:
            if (datalink->hdr.len == sizeof(network_msg_chlor_name_t)) {
                network->typ = NETWORK_MSG_TYP_CHLOR_NAME;
                network->u.chlor_name = (network_msg_chlor_name_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_LEVEL_SET:
            if (datalink->hdr.len == sizeof(network_msg_chlor_level_set_t)) {
                network->typ = NETWORK_MSG_TYP_CHLOR_LEVEL_SET;
                network->u.chlor_level_set = (network_msg_chlor_level_set_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_LEVEL_RESP:
            if (datalink->hdr.len == sizeof(network_msg_chlor_level_resp_t)) {
                network->typ = NETWORK_MSG_TYP_CHLOR_LEVEL_RESP;
                network->u.chlor_level_resp = (network_msg_chlor_level_resp_t *) datalink->data;
            }
            break;
        case DATALINK_CHLOR_TYP_X14:
            // silently ignore
            break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR == 'y') {
                ESP_LOGW(TAG, "unknown IC typ %u", datalink->hdr.typ);
            }
            break;
    }
}

bool
network_rx_msg(datalink_pkt_t const * const datalink, network_msg_t * const network, bool * const txOpportunity)
{
	network_addrgroup_t src = network_groupaddr(datalink->hdr.src);
	network_addrgroup_t dst = network_groupaddr(datalink->hdr.dst);

	if (dst == NETWORK_ADDRGROUP_X09) {
		return false; // stick our head in the sand
	}

	name_reset_idx();

	switch (datalink->proto) {
		case DATALINK_PROT_A5:
			if (src == NETWORK_ADDRGROUP_PUMP || dst == NETWORK_ADDRGROUP_PUMP) {
                _decode_msg_a5_pump(datalink, network);
			} else {
                _decode_msg_a5_ctrl(datalink, network);
			}
			break;
		case DATALINK_PROT_IC:
			if (dst == NETWORK_ADDRGROUP_ALL || dst == NETWORK_ADDRGROUP_CHLOR) {
                _decode_msg_ic_chlor(datalink, network);
			}
			break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR == 'y') {
                ESP_LOGW(TAG, "unknown proto %u", datalink->proto);
            }
	}
    *txOpportunity =
        datalink->proto == DATALINK_PROT_A5 &&
        network_groupaddr(datalink->hdr.src) == NETWORK_ADDRGROUP_CTRL &&
        network_groupaddr(datalink->hdr.dst) == NETWORK_ADDRGROUP_ALL;

    return network->typ != NETWORK_MSG_TYP_NONE;
}
