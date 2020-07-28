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

str_value_name_pair_t _typ_ctrls[] = {
#define XX(num, name) { .typ = num, .str = #name },
 NETWORK_TYP_CTRL_MAP(XX)
#undef XX
};

const char *
datalink_typ_ctrl_str(network_typ_ctrl_t typ)
{
    str_value_name_pair_t const * pair = _typ_ctrls;
    for(uint ii = 0; ii < ARRAY_SIZE(_typ_ctrls); ii++, pair++) {
        if (pair->typ == typ) {
            return pair->str;
        }
    }
    return hex8_str(typ);
}

str_value_name_pair_t _typ_pumps[] = {
#define XX(num, name) { .typ = num, .str = #name },
  NETWORK_TYP_PUMP_MAP(XX)
#undef XX
};

const char *
datalink_typ_pump_str(network_typ_pump_t typ)
{
    str_value_name_pair_t const * pair = _typ_pumps;
    for(uint ii = 0; ii < ARRAY_SIZE(_typ_pumps); ii++, pair++) {
        if (pair->typ == typ) {
            return pair->str;
        }
    }
    return hex8_str(typ);
}

str_value_name_pair_t _typ_chlors[] = {
#define XX(num, name) { .typ = num, .str = #name },
  NETWORK_TYP_CHLOR_MAP(XX)
#undef XX
};

const char *
datalink_typ_chlor_str(network_typ_chlor_t typ)
{
    str_value_name_pair_t const * pair = _typ_chlors;
    for(uint ii = 0; ii < ARRAY_SIZE(_typ_chlors); ii++, pair++) {
        if (pair->typ == typ) {
            return pair->str;
        }
    }
    return hex8_str(typ);
}
