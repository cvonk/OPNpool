/**
 * @brief Data Link layer: bytes from the RS485 transceiver to/from data packets
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "../rs485/rs485.h"
#include "../network/network_hdr.h"
#include "skb/skb.h"
#include "datalink.h"

// static char const * const TAG = "datalink";

datalink_preamble_a5_t datalink_preamble_a5 = { 0x00, 0xFF, 0xA5 };  // use 0xA5 in the preamble to detection more reliable
datalink_preamble_ic_t datalink_preamble_ic = { 0x10, 0x02 };

datalink_addrgroup_t
datalink_groupaddr(uint16_t const addr)
{
	return (datalink_addrgroup_t)(addr >> 4);
}

uint8_t
datalink_devaddr(uint8_t group, uint8_t const id)
{
	return (group << 4) | id;
}

uint16_t
datalink_calc_crc(uint8_t const * const start, uint8_t const * const stop)
{
    uint16_t crc = 0;
    for (uint8_t const * byte = start; byte < stop; byte++) {
        crc += *byte;
    }
    return crc;
}
