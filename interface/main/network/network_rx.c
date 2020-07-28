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

typedef struct hdr_data_hdr_copy_t {
    uint8_t dst;  // destination
    uint8_t src;  // source
    uint8_t typ;  // message type
} hdr_data_hdr_copy_t;

static void
_decode_msg_a5_ctrl(uint8_t const * const data, size_t const data_len, hdr_data_hdr_copy_t const * const hdr_copy, network_msg_t * const network)
{
    network->typ = MSG_TYP_NONE;

    switch (hdr_copy->typ) {

        case NETWORK_TYP_CTRL_SET_ACK:
            if (data_len == sizeof(network_msg_ctrl_set_ack_t)) {
                network->typ = MSG_TYP_CTRL_SET_ACK;
                network->u.ctrl_set_ack = (network_msg_ctrl_set_ack_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_CIRCUIT_SET:
            if (data_len == sizeof(network_msg_ctrl_circuit_set_t)) {
                network->typ = MSG_TYP_CTRL_CIRCUIT_SET;
                network->u.ctrl_circuit_set = (network_msg_ctrl_circuit_set_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_SCHED_REQ:
            if (data_len == 0) {
                network->typ = MSG_TYP_CTRL_SCHED_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_SCHED:
            if (data_len == sizeof(network_msg_ctrl_sched_resp_t)) {
                network->typ = MSG_TYP_CTRL_SCHED_RESP;
                network->u.ctrl_sched_resp = (network_msg_ctrl_sched_resp_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_STATE_REQ:
            if (data_len == 0) {
                network->typ = MSG_TYP_CTRL_STATE_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_STATE:
            if (data_len == sizeof(network_msg_ctrl_state_t)) {
                network->typ = MSG_TYP_CTRL_STATE;
                network->u.ctrl_state = (network_msg_ctrl_state_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_STATE_SET:
            // unfinished
            if (data_len == sizeof(network_msg_ctrl_state_set_t)) {
                network->typ = MSG_TYP_CTRL_STATE_SET;
                network->u.ctrl_state = (network_msg_ctrl_state_set_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_TIME_REQ:
            if (data_len == 0) {
                network->typ = MSG_TYP_CTRL_TIME_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_TIME:
            if (data_len == sizeof(network_msg_ctrl_time_t)) {
                network->typ = MSG_TYP_CTRL_TIME;
                network->u.ctrl_time = (network_msg_ctrl_time_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_TIME_SET:
            if (data_len == sizeof(network_msg_ctrl_time_t)) {
                network->typ = MSG_TYP_CTRL_TIME_SET;
                network->u.ctrl_time = (network_msg_ctrl_time_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT_REQ:
            if (data_len == 0) {
                network->typ = MSG_TYP_CTRL_HEAT_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT:
            if (data_len == sizeof(network_msg_ctrl_heat_t)) {
                network->typ = MSG_TYP_CTRL_HEAT;
                network->u.ctrl_heat = (network_msg_ctrl_heat_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_HEAT_SET:
            if (data_len == sizeof(network_msg_ctrl_heat_set_t)) {
                network->typ = MSG_TYP_CTRL_HEAT_SET;
                network->u.ctrl_heat_set = (network_msg_ctrl_heat_set_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_LAYOUT_REQ:
            if (data_len == 0) {
                network->typ = MSG_TYP_CTRL_LAYOUT_REQ;
            }
            break;
        case NETWORK_TYP_CTRL_LAYOUT:
            if (data_len == sizeof(network_msg_ctrl_layout_t)) {
                network->typ = MSG_TYP_CTRL_LAYOUT;
                network->u.ctrl_layout = (network_msg_ctrl_layout_t *) data;
            }
            break;
        case NETWORK_TYP_CTRL_LAYOUT_SET:
            if (data_len == sizeof(network_msg_ctrl_layout_set_t)) {
                network->typ = MSG_TYP_CTRL_LAYOUT_SET;
                network->u.ctrl_layout_set = (network_msg_ctrl_layout_set_t *) data;
            }
            break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR) {
                ESP_LOGW(TAG, "unknown A5 ctrl typ (0x%02X)", hdr_copy->typ);
            }
            break;
    }
};

static void
_decode_msg_a5_pump(uint8_t const * const data, size_t const data_len, hdr_data_hdr_copy_t const * const hdr_copy, network_msg_t * const network)
{
    network->typ = MSG_TYP_NONE;
	bool toPump = (datalink_groupaddr(hdr_copy->dst) == DATALINK_ADDRGROUP_PUMP);

    switch (hdr_copy->typ) {
        case NETWORK_TYP_PUMP_REG:
            if (toPump) {
                if (data_len == sizeof(network_msg_pump_reg_set_t)) {
                    network->typ = MSG_TYP_PUMP_REG_SET;
                    network->u.pump_reg_set = (network_msg_pump_reg_set_t *) data;
                }
            } else {
                if (data_len == sizeof(network_msg_pump_reg_resp_t)) {
                    network->typ = MSG_TYP_PUMP_REG_RESP;
                    network->u.pump_reg_set_resp = (network_msg_pump_reg_resp_t *) data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_CTRL:
            if (toPump) {
                if (data_len == sizeof(network_msg_pump_ctrl_t)) {
                    network->typ = MSG_TYP_PUMP_CTRL_SET;
                    network->u.pump_ctrl = (network_msg_pump_ctrl_t *) data;
                }
            } else {
                if (data_len == sizeof(network_msg_pump_ctrl_t)) {
                    network->typ = MSG_TYP_PUMP_CTRL_RESP;
                    network->u.pump_ctrl = (network_msg_pump_ctrl_t *) data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_MODE:
            if (toPump) {
                if (data_len == sizeof(network_msg_pump_mode_t)) {
                    network->typ = MSG_TYP_PUMP_MODE_SET;
                    network->u.pump_mode = (network_msg_pump_mode_t *) data;
                }
            } else {
                if (data_len == sizeof(network_msg_pump_mode_t)) {
                    network->typ = MSG_TYP_PUMP_MODE_RESP;
                    network->u.pump_mode = (network_msg_pump_mode_t *) data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_RUN:
            if (toPump) {
                if (data_len == sizeof(network_msg_pump_run_t)) {
                    network->typ = MSG_TYP_PUMP_RUN_SET;
                    network->u.pump_run = (network_msg_pump_run_t *) data;
                }
            } else {
                if (data_len == sizeof(network_msg_pump_run_t)) {
                    network->typ = MSG_TYP_PUMP_RUN_RESP;
                    network->u.pump_run = (network_msg_pump_run_t *) data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_STATUS:
            if (toPump) {
                if (data_len == 0) {
                    network->typ = MSG_TYP_PUMP_STATUS_REQ;
                }
            } else {
                if (data_len == sizeof(network_msg_pump_status_resp_t)) {
                    network->typ = MSG_TYP_PUMP_STATUS_RESP;
                    network->u.pump_status_resp = (network_msg_pump_status_resp_t *) data;
                }
            }
            break;
        case NETWORK_TYP_PUMP_0xFF:
            // silently ignore
            break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR) {
                ESP_LOGW(TAG, "unknown A5 pump typ %u", hdr_copy->typ);
            }
            break;
    }
}

static void
_decode_msg_ic_chlor(uint8_t const * const data, size_t const data_len, hdr_data_hdr_copy_t const * const hdr_copy, network_msg_t * const network)
{
    network->typ = MSG_TYP_NONE;

    switch (hdr_copy->typ) {
        case NETWORK_TYP_CHLOR_PING_REQ:
            if (data_len == sizeof(network_msg_chlor_ping_req_t)) {
                network->typ = MSG_TYP_CHLOR_PING_REQ;
                network->u.chlor_ping_req = (network_msg_chlor_ping_req_t *) data;
            }
            break;
        case NETWORK_TYP_CHLOR_PING_RESP:
            if (data_len == sizeof(network_msg_chlor_ping_resp_t)) {
                network->typ = MSG_TYP_CHLOR_PING_RESP;
                network->u.chlor_ping = (network_msg_chlor_ping_resp_t *) data;
            }
            break;
        case NETWORK_TYP_CHLOR_NAME:
            if (data_len == sizeof(network_msg_chlor_name_t)) {
                network->typ = MSG_TYP_CHLOR_NAME;
                network->u.chlor_name = (network_msg_chlor_name_t *) data;
            }
            break;
        case NETWORK_TYP_CHLOR_LEVEL_SET:
            if (data_len == sizeof(network_msg_chlor_level_set_t)) {
                network->typ = MSG_TYP_CHLOR_LEVEL_SET;
                network->u.chlor_level_set = (network_msg_chlor_level_set_t *) data;
            }
            break;
        case NETWORK_TYP_CHLOR_LEVEL_RESP:
            if (data_len == sizeof(network_msg_chlor_level_resp_t)) {
                network->typ = MSG_TYP_CHLOR_LEVEL_RESP;
                network->u.chlor_level_resp = (network_msg_chlor_level_resp_t *) data;
            }
            break;
        case NETWORK_TYP_CHLOR_X14:
            // silently ignore
            break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR) {
                ESP_LOGW(TAG, "unknown IC typ %u", hdr_copy->typ);
            }
            break;
    }
}

void
_peek_datalink_hdr(skb_t * const skb, datalink_prot_t const prot, hdr_data_hdr_copy_t * const hdr_copy)
{
    switch (prot) {
		case DATALINK_PROT_IC: {
            size_t const head_size = sizeof(datalink_head_ic_t);
            datalink_head_ic_t const * const head = (datalink_head_ic_t *) skb_push(skb, head_size);
            hdr_copy->typ = head->hdr.typ;
            hdr_copy->src =
            hdr_copy->dst = 0;
            skb_call(skb, head_size);
            break;
        }
		case DATALINK_PROT_A5_CTRL:
		case DATALINK_PROT_A5_PUMP: {
            size_t const head_size = sizeof(datalink_head_a5_t);
            datalink_head_a5_t const * const head = (datalink_head_a5_t *) skb_push(skb, head_size);
            hdr_copy->typ = head->hdr.typ;
            hdr_copy->src = head->hdr.src;
            hdr_copy->dst = head->hdr.dst;
            skb_call(skb, head_size);
            break;
        }
    }
}

bool
network_rx_msg(datalink_pkt_t const * const pkt, network_msg_t * const msg, bool * const txOpportunity)
{
	name_reset_idx();

    hdr_data_hdr_copy_t hdr_copy;
    _peek_datalink_hdr(pkt->skb, pkt->prot, &hdr_copy);

	switch (pkt->prot) {
		case DATALINK_PROT_A5_CTRL:
            _decode_msg_a5_ctrl(pkt->data, pkt->data_len, &hdr_copy, msg);
			break;
		case DATALINK_PROT_A5_PUMP:
            _decode_msg_a5_pump(pkt->data, pkt->data_len, &hdr_copy, msg);
			break;
		case DATALINK_PROT_IC:
            _decode_msg_ic_chlor(pkt->data, pkt->data_len, &hdr_copy, msg);
			break;
        default:
            if (CONFIG_POOL_DBG_NETWORK_ONERROR) {
                ESP_LOGW(TAG, "unknown proto %u", pkt->prot);
            }
	}
    *txOpportunity =
        pkt->prot == DATALINK_PROT_A5_CTRL &&
        datalink_groupaddr(hdr_copy.src) == DATALINK_ADDRGROUP_CTRL &&
        datalink_groupaddr(hdr_copy.dst) == DATALINK_ADDRGROUP_ALL;

    return msg->typ != MSG_TYP_NONE;
}
