/**
 * @brief Data Link layer: encode messages to data link layer packets
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
#include "../tx_buf/tx_buf.h"
#include "network.h"

// static char const * const TAG = "network_tx";

tx_buf_handle_t
_alloc_txb_a5(size_t const msg_size)
{
    tx_buf_handle_t const txb = alloc_txb(DATALINK_A5_HEAD_SIZE + msg_size + DATALINK_A5_TAIL_SIZE);
    tx_buf_reserve(txb, DATALINK_A5_HEAD_SIZE);
    return txb;
}

void
network_tx_circuit_set(rs485_handle_t const rs485_handle, uint8_t circuit, uint8_t value)
{
    size_t msg_size = sizeof(network_msg_ctrl_circuit_set_t);
    tx_buf_handle_t const txb = _alloc_txb_a5(msg_size);

    network_msg_ctrl_circuit_set_t * const msg = tx_buf_put(txb, msg_size);
    msg->circuit = circuit;
    msg->value = value;
    datalink_tx_pkt(rs485_handle, txb, DATALINK_PROT_A5_CTRL, DATALINK_CTRL_TYP_CIRCUIT_SET);  // will free when done
}



#if 0
void
EncodeA5::circuitMsg(element_t * element, uint_least8_t const circuit, uint_least8_t const value)
{
	*element = {
		.typ = DATALINK_CTRL_TYP_circuitSet,
		.dataLen = sizeof(network_msg_ctrl_circuit_set_t),
		.data = {
			.circuitSet = {
				.circuit = circuit,
				.value = value
			}
		}
	};
}

void
EncodeA5::heatMsg(element_t * element, uint8_t const poolTempSetpoint, uint8_t const spaTempSetpoint, uint8_t const heatSrc)
{
	element->typ = DATALINK_CTRL_TYP_heatSet;
	element->dataLen = sizeof(network_msg_ctrl_heat_set_t);
	element->data.heatSet = {
		.poolTempSetpoint = poolTempSetpoint,
		.spaTempSetpoint = spaTempSetpoint,
		.heatSrc = heatSrc,
		.UNKNOWN_3 = {}
	};
}

#if 0
void
EncodeA5::setTime(JsonObject * root, network_msg_t * sys, datalink_a5_hdr_t * const hdr, struct network_msg_ctrl_time_t * const msg)
{
	*msg = {
		.hour = 14,
		.minute = 49,
		.UNKNOWN_2 = 0x00,
		.day = 23,
		.month = 5,
		.year = 15,
	};
	send_a5(root, sys, datalink_ctrl_typ_timeSet, hdr, (uint8_t *)msg, sizeof(*msg));
}
#endif

#endif