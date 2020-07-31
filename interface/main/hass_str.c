/**
* @brief packet_task, packetizes RS485 byte stream from Pentair bus
 *
 * The Pentair controller uses two different protocols to communicate with its peripherals:
 *   - 	A5 has messages such as 0x00 0xFF <ldb> <sub> <dst> <src> <cfi> <len> [<data>] <chH> <ckL>
 *   -  IC has messages such as 0x10 0x02 <data0> <data1> <data2> .. <dataN> <ch> 0x10 0x03
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>

#include "utils/utils.h"
#include "hass_task.h"

static const char * const _dev_types[] = {
#define XX(num, name) #name,
  HASS_DEV_TYP_MAP(XX)
#undef XX
};

char const *
hass_dev_typ_str(hass_dev_typ_t const dev_typ)
{
    return ELEM_AT(_dev_types, dev_typ, hex8_str(dev_typ));
}
