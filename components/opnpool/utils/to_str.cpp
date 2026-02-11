/**
 * @file to_str.cpp
 * @brief Helper functions for converting values to strings using a fixed buffer.
 *
 * @details
 * Provides lightweight, allocation-free string conversion utilities for unsigned integers
 * and booleans, optimized for embedded environments. All conversions use a shared
 * fixed-size buffer to minimize memory usage and avoid dynamic allocation. These
 * functions are used throughout the OPNpool component for logging, diagnostics, and
 * protocol message formatting.
 *
 * ESPHome operates in a single-threaded environment, so explicit thread safety measures
 * are not required within the pool_task context.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Copyright (c) 2014, 2019, 2022, 2026 Coert Vonk
 * @license SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <esp_system.h>
#include <esp_types.h>
#include <string.h>

#include "to_str.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

namespace esphome {
namespace opnpool {

name_str_t name_str = {
    .str = {0},
    .idx = 0,
    .noMem = "sNOMEM", // increase size of str.str[]
    .digits = "0123456789ABCDEF",
};

/**
 * @brief Resets the index of the shared string buffer used for value-to-string conversions.
 *
 * @details
 * This function should be called periodically to avoid running out of buffer space.
 */
void
name_reset_idx()
{
    name_str.idx = 0;
}

/**
 * @brief                Get a string representation of a bool
 *
 * @param value          The bool value to convert
 * @return char const *  A string representing the value ("true" or "false")
 */
char const *
bool_str(bool const value)
{
    char const * const value_str = value ? "true" : "false";
    uint8_t const len = strlen(value_str) + 1;  // +1 for null terminator
    if (name_str.idx + len >= ARRAY_SIZE(name_str.str)) {
        return name_str.noMem;
    }
    char * s = name_str.str + name_str.idx;
    strcpy(s, value_str);
    name_str.idx += len;
    return s;
}

/**
 * @brief                Get a hexadecimal string representation of a uint8_t
 *
 * @param value          The uint8_t value to convert
 * @return char const *  A string representing the value
 */
char const *
uint8_str(uint8_t const value)
{
    uint_least8_t const nrdigits = sizeof(value) << 1;

    if (name_str.idx + nrdigits + 1U >= ARRAY_SIZE(name_str.str)) {
        return name_str.noMem;  // increase size of str.str[]
    }
    char * s = name_str.str + name_str.idx;
    s[0] = name_str.digits[(value & 0xF0) >> 4];
    s[1] = name_str.digits[(value & 0x0F)];
    s[nrdigits] = '\0';
    name_str.idx += nrdigits + 1U;
    return s;
}

/**
 * @brief                Get a hexadecimal string representation of a uint16_t
 *
 * @param value          The uint16_t value to convert
 * @return char const *  A string representing the value
 */
char const *
uint16_str(uint16_t const value)
{
    uint_least8_t const nrdigits = sizeof(value) << 1;

    if (name_str.idx + nrdigits + 1U >= ARRAY_SIZE(name_str.str)) {
        return name_str.noMem;  // to prevent this, increase the size of str.str[]
    }
    char * s = name_str.str + name_str.idx;
    s[0] = name_str.digits[(value & 0xF000) >> 12];
    s[1] = name_str.digits[(value & 0x0F00) >>  8];
    s[2] = name_str.digits[(value & 0x00F0) >>  4];
    s[3] = name_str.digits[(value & 0x000F)];
    s[nrdigits] = '\0';
    name_str.idx += nrdigits + 1U;
    return s;
}

/**
 * @brief                Get a hexadecimal string representation of a uint32_t
 *
 * @param value          The uint32_t value to convert
 * @return char const *  A string representing the value
 */
char const *
uint32_str(uint32_t const value)
{
    uint_least8_t const nrdigits = sizeof(value) << 1;

    if (name_str.idx + nrdigits + 1U >= ARRAY_SIZE(name_str.str)) {
        return name_str.noMem;  // to prevent this, increase the size of str.str[]
    }
    char * s = name_str.str + name_str.idx;
    s[0] = name_str.digits[(value & 0xF0000000) >> 28];
    s[1] = name_str.digits[(value & 0x0F000000) >> 24];
    s[2] = name_str.digits[(value & 0x00F00000) >> 20];
    s[3] = name_str.digits[(value & 0x000F0000) >> 16];
    s[4] = name_str.digits[(value & 0x0000F000) >> 12];
    s[5] = name_str.digits[(value & 0x00000F00) >>  8];
    s[6] = name_str.digits[(value & 0x000000F0) >>  4];
    s[7] = name_str.digits[(value & 0x0000000F)];
    s[nrdigits] = '\0';
    name_str.idx += nrdigits + 1U;
    return s;
}

/**
 * @brief                Get a string representation of the controller date
 *
 * @param year           The year of the date
 * @param month          The month of the date
 * @param day            The day of the date
 * @return char const *  A string representing the date
 */
char const *
date_str(uint16_t const year, uint8_t const month, uint8_t const day)
{
    size_t const len = 14;  // worst case: "65535-255-255\0"
    if (name_str.idx + len >= ARRAY_SIZE(name_str.str)) {
        return name_str.noMem;  // increase size of str.str[]
    }
    char * s = name_str.str + name_str.idx;
    snprintf(s, len, "%04u-%02u-%02u", 2000 + year, month, day);
    name_str.idx += len;
    return s;
}

/**
 * @brief                Get a string representation of the controller time
 *
 * @param hour           The hour of the time
 * @param minute         The minute of the time
 * @return char const *  A string representing the time
 */
char const *
time_str(uint8_t const hour, uint8_t const minute)
{
    size_t const len = 8;  // worst case: "255:255\0"
    if (name_str.idx + len >= ARRAY_SIZE(name_str.str)) {
        return name_str.noMem;  // increase size of str.str[]
    }
    char * s = name_str.str + name_str.idx;
    snprintf(s, len, "%02u:%02u", hour, minute);
    name_str.idx += len;
    return s;
}

/**
 * @brief                Get a string representation of the controller version number
 *
 * @param major          The major version number
 * @param minor          The minor version number
 * @return char const *  A string representing the version number
 */
char const *
version_str(uint8_t const major, uint8_t const minor)
{
    size_t const len = 8;  // "MMM.mmm\0"
    if (name_str.idx + len >= ARRAY_SIZE(name_str.str)) {
        return name_str.noMem;  // increase size of str.str[]
    }
    char * s = name_str.str + name_str.idx;
    snprintf(s, len, "%u.%u", major, minor);
    name_str.idx += len;
    return s;
}

} // namespace opnpool
} // namespace esphome