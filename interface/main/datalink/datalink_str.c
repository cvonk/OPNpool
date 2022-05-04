/**
 * @brief OPNpool - Data Link layer: support, enum to string
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
 * SPDX-FileCopyrightText: Copyright 2014,2019,2022 Coert Vonk
 */

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
