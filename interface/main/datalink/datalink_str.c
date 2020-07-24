/**
* @brief datalink layer: enum to string
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "../utils/utils_str.h"
#include "datalink.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

str_value_name_pair_t _a5_ctrl_msgtyps[] = {
#define XX(num, name) { .typ = num, .str = #name },
 DATALINK_CTRL_TYP_MAP(XX)
#undef XX
};

const char *
datalink_ctrl_type_str(datalink_ctrl_typ_t typ)
{
    str_value_name_pair_t const * pair = _a5_ctrl_msgtyps;
    for(uint ii = 0; ii < ARRAY_SIZE(_a5_ctrl_msgtyps); ii++, pair++) {
        if (pair->typ == typ) {
            return pair->str;
        }
    }
    return hex8_str(typ);
}

str_value_name_pair_t _a5_pump_msgtyps[] = {
#define XX(num, name) { .typ = num, .str = #name },
  DATALINK_PUMP_TYP_MAP(XX)
#undef XX
};

const char *
datalink_pump_typ_str(datalink_pump_typ_t typ)
{
    str_value_name_pair_t const * pair = _a5_pump_msgtyps;
    for(uint ii = 0; ii < ARRAY_SIZE(_a5_pump_msgtyps); ii++, pair++) {
        if (pair->typ == typ) {
            return pair->str;
        }
    }
    return hex8_str(typ);
}

str_value_name_pair_t _ic_chlor_msgtyps[] = {
#define XX(num, name) { .typ = num, .str = #name },
  DATALINK_CHLOR_TYP_MAP(XX)
#undef XX
};

const char *
datalink_chlor_typ_str(datalink_chlor_typ_t typ)
{
    str_value_name_pair_t const * pair = _ic_chlor_msgtyps;
    for(uint ii = 0; ii < ARRAY_SIZE(_ic_chlor_msgtyps); ii++, pair++) {
        if (pair->typ == typ) {
            return pair->str;
        }
    }
    return hex8_str(typ);
}
