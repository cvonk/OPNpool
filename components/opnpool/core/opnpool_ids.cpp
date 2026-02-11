/**
 * @file opnpool_ids.cpp
 * @brief Entity ID conversion utilities for OPNpool component.
 *
 * @details
 * Implements conversion functions between ESPHome entity IDs (climate_id_t, switch_id_t)
 * and their corresponding internal representations (poolstate_thermo_typ_t,
 * network_pool_circuit_t). These mappings enable the component to translate between
 * ESPHome's entity model and the pool controller's protocol model.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>

#include "opnpool_ids.h"
#include "pool_task/network_msg.h"
#include "poolstate.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

/**
 * @brief Convert climate_id_t to poolstate_thermo_typ_t.
 *
 * @details
 * This helper assumes that climate_id_t and poolstate_thermo_typ_t enums are kept in the
 * same order and size. A static_assert is used to enforce the size match at compile time.
 *
 * @param[in] id climate_id_t value to convert.
 * @return       poolstate_thermo_typ_t corresponding value.
 */
[[nodiscard]] poolstate_thermo_typ_t
climate_id_to_poolstate_thermo(climate_id_t const id)
{
    static_assert(enum_count<climate_id_t>() == enum_count<poolstate_thermo_typ_t>(), "climate_id_t and poolstate_thermo_typ_t must have the same number of elements");
    static_assert(enum_index(climate_id_t::SPA_CLIMATE) == enum_index(poolstate_thermo_typ_t::SPA), "climate_id_t and poolstate_thermo_typ_t must have matching elements");
    static_assert(enum_index(climate_id_t::POOL_CLIMATE) == enum_index(poolstate_thermo_typ_t::POOL), "climate_id_t and poolstate_thermo_typ_t must have matching elements");

    return static_cast<poolstate_thermo_typ_t>(static_cast<uint8_t>(id));
}

/**
 * @brief Convert switch_id_t to network_pool_circuit_t.
 *
 * @details
 * This helper assumes that switch_id_t and network_pool_circuit_t enums are kept in the
 * same order and size. A static_assert is used to enforce the size match at compile time.
 *
 * @param[in] id switch_id_t value to convert.
 * @return       network_pool_circuit_t corresponding value.
 */
[[nodiscard]] network_pool_circuit_t
switch_id_to_network_circuit(switch_id_t const id)
{
    static_assert(enum_count<switch_id_t>()         == enum_count<network_pool_circuit_t>(),         "switch_id_t and network_pool_circuit_t must have the same number of elements");
    static_assert(enum_index(switch_id_t::SPA)      == enum_index(network_pool_circuit_t::SPA),      "switch_id_t and network_pool_circuit_t must have matching elements");
    static_assert(enum_index(switch_id_t::POOL)     == enum_index(network_pool_circuit_t::POOL),     "switch_id_t and network_pool_circuit_t must have matching elements");
    static_assert(enum_index(switch_id_t::AUX1)     == enum_index(network_pool_circuit_t::AUX1),     "switch_id_t and network_pool_circuit_t must have matching elements");
    static_assert(enum_index(switch_id_t::AUX2)     == enum_index(network_pool_circuit_t::AUX2),     "switch_id_t and network_pool_circuit_t must have matching elements");
    static_assert(enum_index(switch_id_t::AUX3)     == enum_index(network_pool_circuit_t::AUX3),     "switch_id_t and network_pool_circuit_t must have matching elements"); 
    static_assert(enum_index(switch_id_t::FEATURE1) == enum_index(network_pool_circuit_t::FEATURE1), "switch_id_t and network_pool_circuit_t must have matching elements");
    static_assert(enum_index(switch_id_t::FEATURE2) == enum_index(network_pool_circuit_t::FEATURE2), "switch_id_t and network_pool_circuit_t must have matching elements");
    static_assert(enum_index(switch_id_t::FEATURE3) == enum_index(network_pool_circuit_t::FEATURE3), "switch_id_t and network_pool_circuit_t must have matching elements");
    static_assert(enum_index(switch_id_t::FEATURE4) == enum_index(network_pool_circuit_t::FEATURE4), "switch_id_t and network_pool_circuit_t must have matching elements");

    return static_cast<network_pool_circuit_t>(static_cast<uint8_t>(id));
}

} // namespace opnpool
} // namespace esphome
