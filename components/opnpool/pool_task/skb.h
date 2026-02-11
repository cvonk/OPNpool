/**
 * @file skb.h
 * @brief Linux sk_buff inspired socket buffer for zero-copy packet handling.
 *
 * @details
 * This module provides a socket buffer (skb) abstraction inspired by Linux's sk_buff,
 * enabling efficient packet construction and parsing without data copying. The buffer
 * maintains four internal pointers (head, data, tail, end) that define the buffer's
 * structure and allow protocol layers to prepend headers or strip them as packets
 * traverse the network stack.
 *
 * Key operations:
 * - skb_reserve(): Reserve headroom for protocol headers to be added later
 * - skb_put(): Append data to the buffer (moves tail forward)
 * - skb_push(): Prepend a header (moves data backward toward head)
 * - skb_pull(): Strip a header (moves data forward toward tail)
 * - skb_trim(): Shrink data from the end (moves tail backward)
 *
 * This design allows building packets from the inside out (payload first, then headers)
 * and parsing packets by progressively stripping headers, all without copying data.
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

/// @name Socket Buffer Structures
/// @brief Internal structures for socket buffer management.
/// @{

/**
 * @brief Internal pointer structure for socket buffer management.
 *
 * @details
 * Maintains the four pointers that define the buffer's structure:
 * - head: Start of allocated buffer (never changes after allocation)
 * - data: Start of current payload (moves with push/pull operations)
 * - tail: End of current payload (moves with put/trim operations)
 * - end: End of allocated buffer (never changes after allocation)
 */
struct skb_priv_t {
    uint8_t * head;  ///< Start of the allocated packet buffer.
    uint8_t * data;  ///< Start of the current payload data.
    uint8_t * tail;  ///< End of the current payload data.
    uint8_t * end;   ///< End of the allocated packet buffer.
};

/**
 * @brief Socket buffer structure for zero-copy packet handling.
 *
 * @details
 * Contains the private pointer structure, length/size metadata, and a flexible
 * array member for the actual buffer data. Allocated as a single contiguous
 * block of memory to minimize fragmentation and cache misses.
 */
struct skb_t {
    skb_priv_t priv;   ///< Internal pointer structure.
    size_t     len;    ///< Amount of data currently in buffer.
    size_t     size;   ///< Total allocated buffer capacity.
    uint8_t    buf[];  ///< Flexible array member for buffer data.
};

/// @}

/// @name Type Aliases
/// @brief Handle type for socket buffer operations.
/// @{

/// @brief Handle to a socket buffer instance.
using skb_handle_t = skb_t *;

/// @}

/// @name Allocation and Deallocation
/// @brief Functions for allocating and freeing socket buffers.
/// @{

/**
 * @brief Allocates a new socket buffer with the given size.
 *
 * @details
 * Creates and initializes a socket buffer structure. Allocates a contiguous block
 * of memory large enough to hold both the skb_t metadata structure and the
 * requested buffer size.
 *
 * @param[in] size The size of the buffer to allocate.
 * @return         Handle to the allocated skb, or nullptr on failure.
 */
[[nodiscard]] skb_handle_t skb_alloc(size_t const size);

/**
 * @brief Frees a socket buffer.
 *
 * @details
 * Releases the memory allocated for a socket buffer and its associated data area.
 * The caller should ensure the skb is no longer in use before calling this function,
 * as the handle becomes invalid after this operation completes.
 *
 * @param[in] skb Handle to the skb to free.
 */
void skb_free(skb_handle_t const skb);

/// @}

/// @name Data Pointer Manipulation
/// @brief Functions for manipulating the data area within a socket buffer.
/// @{

/**
 * @brief Reserves headroom for protocol headers.
 *
 * @details
 * Reserves space at the beginning of the buffer for protocol headers to be added later.
 * This function moves both the data and tail pointers forward by the specified amount,
 * creating a gap between the head pointer and the data pointer. This reserved headroom
 * allows lower-layer protocol headers (such as Datalink layer headers) to be prepended
 * later using skb_push() without needing to shift existing data.
 *
 * This function must be called immediately after skb_alloc() and before any data is
 * added to the buffer.
 *
 * @param[in] skb        Handle to the skb to reserve headroom for.
 * @param[in] header_len The length of the header space to reserve.
 */
void skb_reserve(skb_handle_t const skb, size_t const header_len);

/**
 * @brief Appends data to the buffer and returns pointer to write location.
 *
 * @details
 * Expands the used data area at the end of the buffer by moving the tail pointer
 * forward. This function reserves space for new data and returns a pointer to where
 * that data should be written. The caller is responsible for actually writing the
 * data to the returned location.
 *
 * This is typically used when appending payload data to a buffer that may already
 * contain headers. The function updates both the skb's length field and the tail
 * pointer to reflect the expanded data area.
 *
 * @param[in] skb           Handle to the skb to append data to.
 * @param[in] user_data_len The length of the user data area to reserve.
 * @return                  Pointer to the start of the newly reserved data area.
 */
[[nodiscard]] uint8_t * skb_put(skb_handle_t const skb, size_t const user_data_len);

/**
 * @brief Reclaims previously allocated data space from the end.
 *
 * @details
 * Shrinks the used data area at the end of the buffer by moving the tail pointer
 * backward (toward head). This is the inverse operation of skb_put() and is used
 * to reclaim space that was previously reserved.
 *
 * @param[in] skb           Handle to the skb to reclaim space from.
 * @param[in] user_data_adj The number of bytes to reclaim from the end.
 * @return                  Pointer to the start of the (adjusted) data area.
 */
uint8_t * skb_trim(skb_handle_t const skb, size_t const user_data_adj);

/**
 * @brief Prepends protocol header and returns pointer to write location.
 *
 * @details
 * Expands the used data area at the beginning of the buffer by moving the data
 * pointer backward (toward head). This function reserves space for a protocol
 * header and returns a pointer to where that header should be written.
 *
 * This is used in a protocol stack to prepend headers as a packet moves down the
 * layers. The function updates both the skb's length field and the data pointer
 * to reflect the expanded data area.
 *
 * @param[in] skb        Handle to the skb to prepend a header to.
 * @param[in] header_len The length of the header to prepend.
 * @return               Pointer to the start of the newly reserved header space.
 */
[[nodiscard]] uint8_t * skb_push(skb_handle_t const skb, size_t const header_len);

/**
 * @brief Removes protocol header and returns pointer to remaining data.
 *
 * @details
 * Shrinks the used data area at the beginning of the buffer by moving the data
 * pointer forward (toward tail). This is the inverse operation of skb_push() and
 * is used to strip away a processed header as a packet moves up the protocol stack.
 *
 * After calling this function, the returned pointer points to the start of the
 * remaining data (now with the header stripped), which is ready for the next
 * protocol layer to process.
 *
 * @param[in] skb        Handle to the skb to remove a header from.
 * @param[in] header_len The length of the header to remove.
 * @return               Pointer to the start of the remaining data.
 */
[[nodiscard]] uint8_t * skb_pull(skb_handle_t const skb, size_t const header_len);

/**
 * @brief Resets the buffer to its initial state.
 *
 * @details
 * Reinitializes all internal pointers to their original positions, effectively
 * clearing the buffer for reuse. After calling this function, the buffer state
 * matches what it would be immediately after skb_alloc().
 *
 * @param[in] skb Handle to the skb to reset.
 */
void skb_reset(skb_handle_t const skb);

/// @}

/// @name Debugging
/// @brief Functions for debugging and inspecting socket buffers.
/// @{

/**
 * @brief Formats the buffer contents as a hex string.
 *
 * @details
 * Converts the data portion of the buffer (from data pointer to tail pointer)
 * into a human-readable hex string. Each byte is formatted as a two-digit hex
 * value followed by a space.
 *
 * @param[in]  skb      Handle to the skb to format.
 * @param[out] buf      Buffer where the formatted hex string will be written.
 * @param[in]  buf_size The size of the output buffer in bytes.
 * @return              The number of characters written to the buffer.
 */
[[nodiscard]] size_t skb_print(skb_handle_t const skb, char * const buf, size_t const buf_size);

/// @}

}  // namespace opnpool
}  // namespace esphome
