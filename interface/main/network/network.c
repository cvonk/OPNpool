/**
 * @brief Data Link layer: packets from the data link layer to/from messages
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
#include "network.h"

uint8_t
network_ic_len(uint8_t const ic_typ)
{
    switch (ic_typ) {
        case DATALINK_CHLOR_TYP_PING_REQ:
            return sizeof(network_msg_chlor_ping_req_t);
        case DATALINK_CHLOR_TYP_PING_RESP:
            return sizeof(network_msg_chlor_ping_t);
        case DATALINK_CHLOR_TYP_NAME:
            return sizeof(network_msg_chlor_name_t);
        case DATALINK_CHLOR_TYP_LEVEL_SET:
            return sizeof(network_msg_chlor_level_set_t);
        case DATALINK_CHLOR_TYP_LEVEL_RESP:
            return sizeof(network_msg_chlor_ping_t);
        case DATALINK_CHLOR_TYP_X14:
            return sizeof(mChlor0X14_ic_t);
        default:
            return 0;
    };
}
