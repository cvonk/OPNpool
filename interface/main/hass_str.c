/**
 * @brief OPNpool, hass_str
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
