/**
 * @brief Pool state: support, enum/various to string
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>

#include "../utils/utils.h"
#include "poolstate.h"


/**
 * poolstate_chlor_status_t
 **/

static const char * const _network_chlor_states[] = {
#define XX(num, name) #name,
  POOLSTATE_CHLOR_STATUS_MAP(XX)
#undef XX
};

const char *
poolstate_chlor_state_str(poolstate_chlor_status_t const chlor_state)
{
  return ELEM_AT(_network_chlor_states, chlor_state, hex8_str(chlor_state));
}
