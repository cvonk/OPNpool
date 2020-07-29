/**
* @brief Data Link Layer: support, enum to string
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "../utils/utils.h"
#include "datalink.h"
#include "datalink_pkt.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static const char * const _datalink_prots[] = {
#define XX(num, name) #name,
  DATALINK_PROT_MAP(XX)
#undef XX
};

char const *
datalink_prot_str(datalink_prot_t const prot)
{
    return ELEM_AT(_datalink_prots, prot, hex8_str(prot));
}
