/**
 * @brief network layer
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>

#include "presentation.h"

uint8_t
network_ic_len(uint8_t const ic_typ)
{
    switch (ic_typ) {
        case MT_CHLOR_PING_REQ:   return sizeof(mChlorPingReq_ic_t);
        case MT_CHLOR_PING:       return sizeof(mChlorPing_ic_t);
        case MT_CHLOR_NAME:       return sizeof(mChlorName_ic_t);
        case MT_CHLOR_LEVEL_SET:  return sizeof(mChlorLevelSet_ic_t);
        case MT_CHLOR_LEVEL_RESP: return sizeof(mChlorPing_ic_t);
        case MT_CHLOR_0x14:       return sizeof(mChlor0X14_ic_t);
        default:                  return 0;
    };
}

ADDRGROUP_t
network_group_addr(uint16_t const addr)
{
	return (ADDRGROUP_t)(addr >> 4);
}

uint8_t
network_dev_addr(uint8_t group, uint8_t const id)
{
	return (group << 4) | id;
}
