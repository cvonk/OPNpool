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

static const char * const _poolstate_chlor_states[] = {
#define XX(num, name) #name,
  POOLSTATE_CHLOR_STATUS_MAP(XX)
#undef XX
};

const char *
poolstate_chlor_state_str(poolstate_chlor_status_t const chlor_state_id)
{
  return ELEM_AT(_poolstate_chlor_states, chlor_state_id, hex8_str(chlor_state_id));
}

static const char * const _poolstate_thermostats[] = {
#define XX(num, name) #name,
  POOLSTATE_THERMOSTAT_MAP(XX)
#undef XX
};

const char *
poolstate_thermostat_str(poolstate_thermostats_t const thermostat_id)
{
  return ELEM_AT(_poolstate_thermostats, thermostat_id, hex8_str(thermostat_id));
}

static const char * const _poolstate_temps[] = {
#define XX(num, name) #name,
  POOLSTATE_TEMP_MAP(XX)
#undef XX
};

const char *
poolstate_temp_str(poolstate_temps_t const temp_id)
{
  return ELEM_AT(_poolstate_temps, temp_id, hex8_str(temp_id));
}

int
poolstate_temp_nr(char const * const temp_str)
{
    ELEM_POS(_poolstate_temps, temp_str);
}
