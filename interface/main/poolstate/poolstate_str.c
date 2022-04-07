/**
 * @brief OPNpool - Pool state: support, enum/various to string
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

#include "../utils/utils.h"
#include "poolstate.h"


/**
 * poolstate_chlor_status_t
 **/

static const char * const _poolstate_chlor_statuses[] = {
#define XX(num, name) #name,
  POOLSTATE_CHLOR_STATUS_MAP(XX)
#undef XX
};

const char *
poolstate_chlor_status_str(poolstate_chlor_status_t const chlor_state_id)
{
  return ELEM_AT(_poolstate_chlor_statuses, chlor_state_id, hex8_str(chlor_state_id));
}

static const char * const _poolstate_thermos[] = {
#define XX(num, name) #name,
  POOLSTATE_THERMO_TYP_MAP(XX)
#undef XX
};

const char *
poolstate_thermo_str(poolstate_thermo_typ_t const thermostat_id)
{
  return ELEM_AT(_poolstate_thermos, thermostat_id, hex8_str(thermostat_id));
}

int
poolstate_thermo_nr(char const * const thermostat_str)
{
    ELEM_POS(_poolstate_thermos, thermostat_str);
}

static const char * const _poolstate_temps[] = {
#define XX(num, name) #name,
  POOLSTATE_TEMP_TYP_MAP(XX)
#undef XX
};

const char *
poolstate_temp_str(poolstate_temp_typ_t const temp_id)
{
  return ELEM_AT(_poolstate_temps, temp_id, hex8_str(temp_id));
}

int
poolstate_temp_nr(char const * const temp_str)
{
    ELEM_POS(_poolstate_temps, temp_str);
}
