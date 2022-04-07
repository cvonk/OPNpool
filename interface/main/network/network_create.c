/**
 * @brief OPNpool - Network layer: create datalink_pkt from network_msg
 *
 * © Copyright 2014, 2019, 2022, Coert Vonk
 * 
 * This file is part of OPNpool.
 * OPNpool is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * OPNpool is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with OPNpool. 
 * If not, see <https://www.gnu.org/licenses/>.
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>

#include "../datalink/datalink.h"
#include "../datalink/datalink_pkt.h"
#include "../utils/utils.h"
#include "../skb/skb.h"
#include "network.h"

static char const * const TAG = "network_create";

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

/*
 * Create datalink_pkt from network_msg.
 */

bool
network_create_msg(network_msg_t const * const msg, datalink_pkt_t * const pkt)
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
    if (CONFIG_OPNPOOL_DBGLVL_NETWORK > 1) {
        ESP_LOGE(TAG, "unknown msg typ (%u)", msg->typ);
    }
    return false;
}
