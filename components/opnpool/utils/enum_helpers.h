/**
 * @file enum_helpers.h
 * @brief Template helper functions for enum-to-string and string-to-enum conversions
 *
 * @details
 * Provides type-safe template utilities that wrap the magic_enum library for converting
 * between enum values and their string representations. Includes fallback mechanisms for
 * values outside the magic_enum range. The MAGIC_ENUM_RANGE is configured for 0..256 to
 * cover all uint8_t-based protocol enumerations used by the pool controller.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2014, 2019, 2022, 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#ifndef ESPHOME_OPNPOOL_ENUM_HELPERS_H_
#define ESPHOME_OPNPOOL_ENUM_HELPERS_H_
#ifndef __cplusplus
# error "Requires C++ compilation"
#endif

#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/log.h>

#if defined(MAGIC_ENUM_RANGE_MIN)
# undef MAGIC_ENUM_RANGE_MIN
#endif
#if defined(MAGIC_ENUM_RANGE_MAX)
# undef MAGIC_ENUM_RANGE_MAX
#endif
#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include "magic_enum.h"

#include "to_str.h"

namespace esphome {
namespace opnpool {

constexpr char const * const ENUM_HELPER_TAG = "enum_helpers";

/**
 * @brief Convert an enum value to its string representation.
 *
 * @tparam EnumT  Enum type to convert.
 * @param[in] value  The enum value.
 * @return           String name of the enum value, or hex fallback via uint8_str().
 */
template<typename EnumT>
[[nodiscard]] inline const char *
enum_str(EnumT value)
{
    auto name = magic_enum::enum_name(value);
    if (!name.empty()) {
        return name.data();
    }
    return uint8_str(static_cast<uint8_t>(value));  // fallback
}

/**
 * @brief Convert a string to its enum value (as int).
 *
 * @tparam EnumT  Enum type to convert to.
 * @param[in] enum_str  Null-terminated string to look up (case-insensitive).
 * @return              Integer value of the matching enum, or 0 if not found.
 */
template<typename EnumT>
[[nodiscard]] inline int
enum_nr(char const * const enum_str)
{
    if (!enum_str) {
        ESP_LOGE(ENUM_HELPER_TAG, "null to %s", __func__);
        return 0;  // can't return -1, will cause OOB array access
    }
        // try magic_enum first for efficient lookup
    auto value = magic_enum::enum_cast<EnumT>(std::string_view(enum_str), magic_enum::case_insensitive);
    if (value.has_value()) {
        return static_cast<int>(value.value());
    }
        // fallback: search through entire uint8_t range if not found in enum
    for (uint16_t ii = 0; ii <= 0xFF; ii++) {
        auto candidate = static_cast<EnumT>(ii);
        auto name = magic_enum::enum_name(candidate);
        if (!name.empty() && strcasecmp(enum_str, name.data()) == 0) {
            return ii;
        }
    }
    ESP_LOGE(ENUM_HELPER_TAG, "enum_str '%s' not found", enum_str);
    return 0;  // can't return -1, will cause OOB array access
}

/**
 * @brief Return the total number of named values in an enum type.
 *
 * @tparam EnumT  Enum type to query.
 * @return        Number of named enum values.
 */
template<typename EnumT>
[[nodiscard]] constexpr size_t
enum_count() {
    return magic_enum::enum_count<EnumT>();
}

/**
 * @brief Return the underlying integer value of an enum.
 *
 * @tparam E  Enum type (must be an enum).
 * @param[in] e  The enum value.
 * @return       The underlying integer value.
 */
template <typename E>
[[nodiscard]] constexpr auto
enum_index(E e) noexcept -> std::underlying_type_t<E>
{
    static_assert(std::is_enum<E>::value, "E must be an enum type");
    return static_cast<std::underlying_type_t<E>>(e);
}

} // namespace opnpool
} // namespace esphome

#endif // ESPHOME_OPNPOOL_ENUM_HELPERS_H_