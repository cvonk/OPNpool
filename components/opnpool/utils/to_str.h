/**
 * @file to_str.h
 * @brief String conversion utilities for logging and debugging
 *
 * @details
 * Provides functions to convert various data types to string representations
 * using a shared buffer. The buffer is reset via name_reset_idx() before each
 * packet processing cycle.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2014, 2019, 2022, 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#ifndef __cplusplus
# error "Requires C++ compilation"
#endif

#include <esp_system.h>
#include <esp_types.h>

namespace esphome {
namespace opnpool {

    /// Buffer must be at least ((sizeof(datalink_hdr_t) +
    /// sizeof(network_ctrl_state_bcast_t) + 1) * 3 + 50). That is 3 bytes for each hex
    /// value when displaying raw, and another 50 for displaying date/time.
constexpr size_t TO_STR_BUF_SIZE = 200;

/// @brief Reusable global string buffer for to_str conversions.
struct name_str_t {
    char str[TO_STR_BUF_SIZE];  ///< Shared conversion buffer.
    uint_least8_t idx;           ///< Current write index into the buffer.
    char const * const noMem;    ///< Returned when buffer space is exhausted.
    char const * const digits;   ///< Hex digit lookup table.
};

extern name_str_t name_str;

/// @name String Conversion Functions
/// @brief Convert values to string representations in the shared buffer.
/// @{

[[nodiscard]] char const * bool_str(bool const value);
[[nodiscard]] char const * uint8_str(uint8_t const value);
[[nodiscard]] char const * uint16_str(uint16_t const value);
[[nodiscard]] char const * uint32_str(uint32_t const value);
[[nodiscard]] char const * date_str(uint16_t const year, uint8_t const month, uint8_t const day);
[[nodiscard]] char const * time_str(uint8_t const hour, uint8_t const minute);
[[nodiscard]] char const * version_str(uint8_t const major, uint8_t const minor);

/// @}

/// @brief Reset the shared buffer write index. Call before processing each packet.
void name_reset_idx();

} // namespace opnpool
} // namespace esphome