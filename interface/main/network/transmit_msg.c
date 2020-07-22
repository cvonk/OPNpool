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

//#include "pooltypes.h"
//#include "poolstate.h"
//#include "utils.h"
#include "../datalink/datalink.h"
#include "network.h"
#include "name.h"

// static char const * const TAG = "encode";

#if 0
void
EncodeA5::circuitMsg(element_t * element, uint_least8_t const circuit, uint_least8_t const value)
{
	*element = {
		.typ = MT_CTRL_circuitSet,
		.dataLen = sizeof(mCtrlCircuitSet_a5_t),
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
	element->typ = MT_CTRL_heatSet;
	element->dataLen = sizeof(mCtrlHeatSet_a5_t);
	element->data.heatSet = {
		.poolTempSetpoint = poolTempSetpoint,
		.spaTempSetpoint = spaTempSetpoint,
		.heatSrc = heatSrc,
		.UNKNOWN_3 = {}
	};
}

#if 0
void
EncodeA5::setTime(JsonObject * root, network_msg_t * sys, mHdr_a5_t * const hdr, struct mCtrlTime_a5_t * const msg)
{
	*msg = {
		.hour = 14,
		.minute = 49,
		.UNKNOWN_2 = 0x00,
		.day = 23,
		.month = 5,
		.year = 15,
	};
	send_a5(root, sys, MT_CTRL_timeSet, hdr, (uint8_t *)msg, sizeof(*msg));
}
#endif

#endif