/**
 * @brief Data Link layer: encode messages to data link layer packets
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>

#include "../datalink/datalink.h"
#include "../datalink/datalink_pkt.h"
#include "../utils/utils.h"
#include "../skb/skb.h"
#include "network.h"

static char const * const TAG = "network_tx";

skb_handle_t
_skb_alloc_a5(size_t const msg_size)
{
    skb_handle_t const txb = skb_alloc(sizeof(datalink_head_a5_t) + msg_size + sizeof(datalink_tail_a5_t));
    skb_reserve(txb, sizeof(datalink_head_a5_t));
    return txb;
}

typedef struct network_datalink_map_t {
    struct {
        network_msg_typ_t  typ;
        size_t             data_len;
    } network;
    struct {
        datalink_prot_t    prot;
        uint8_t            prot_typ;
    } datalink;
} network_datalink_map_t;

static const network_datalink_map_t _msg_typ_map[] = {
#define XX(num, name, typ, proto, prot_typ) { { MSG_TYP_##name, sizeof(typ)}, {proto, prot_typ} },
  NETWORK_MSG_TYP_MAP(XX)
#undef XX
};

bool
network_tx_msg(network_msg_t const * const msg, datalink_pkt_t * const pkt)
{
    network_datalink_map_t const * map = _msg_typ_map;
    for (uint ii = 0; ii < ARRAY_SIZE(_msg_typ_map); ii++, map++) {
        if (msg->typ == map->network.typ) {
            pkt->prot = map->datalink.prot;
            pkt->prot_typ = map->datalink.prot_typ;
            pkt->data_len = map->network.data_len;
            pkt->skb = skb_alloc(DATALINK_MAX_HEAD_SIZE + map->network.data_len + DATALINK_MAX_TAIL_SIZE);
            skb_reserve(pkt->skb, DATALINK_MAX_HEAD_SIZE);
            pkt->data = skb_put(pkt->skb, map->network.data_len);
            memcpy(pkt->data, msg->u.bytes, map->network.data_len);
            return true;
        }
    }
    if (CONFIG_POOL_DBGLVL_NETWORK > 1) {
        ESP_LOGE(TAG, "unknown msg typ (%u)", msg->typ);
    }
    return false;
}

#if 0
void
network_tx_circuit_set_msg(rs485_handle_t const rs485_handle, uint8_t circuit, uint8_t value)
{
    size_t msg_size = sizeof(network_msg_ctrl_circuit_set_t);
    skb_handle_t const txb = _skb_alloc_a5(msg_size);

    network_msg_ctrl_circuit_set_t * const msg = (network_msg_ctrl_circuit_set_t *) skb_put(txb, msg_size);
    msg->circuit = circuit + 1;
    msg->value = value;
    network_tx_skb(rs485_handle, txb, NETWORK_TYP_CTRL_CIRCUIT_SET);
    //datalink_tx_pkt_queue(rs485_handle, txb, DATALINK_PROT_A5_CTRL, NETWORK_TYP_CTRL_CIRCUIT_SET);  // will free when done
}


void
EncodeA5::circuitMsg(element_t * element, uint_least8_t const circuit, uint_least8_t const value)
{
	*element = {
		.typ = NETWORK_TYP_CTRL_circuitSet,
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
	element->typ = NETWORK_TYP_CTRL_heatSet;
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
EncodeA5::setTime(JsonObject * root, network_msg_t * sys, datalink_hdr_a5_t * const hdr, struct network_msg_ctrl_time_t * const msg)
{
	*msg = {
		.hour = 14,
		.minute = 49,
		.UNKNOWN_2 = 0x00,
		.day = 23,
		.month = 5,
		.year = 15,
	};
	send_a5(root, sys, network_typ_ctrl_timeSet, hdr, (uint8_t *)msg, sizeof(*msg));
}
#endif

#endif