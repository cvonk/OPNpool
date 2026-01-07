#pragma once

/**
 * @brief OPNpool - Linux sk_buff inspired continuous memory
 *
 * Rather than copying packet data at every layer (datalink, network), it passes a
 * pointer to a single `skb`, modifying its internal pointers to "strip" or "add"
 * headers.
 */

#include <esp_system.h>

namespace esphome {
namespace opnpool {

typedef struct skb_priv_t {
    uint8_t * head;  // start of the pkt
    uint8_t * data;  // start of the payload
    uint8_t * tail;  // end of the payload
    uint8_t * end;   // end of the packet
} skb_priv_t;

typedef struct skb_t {
    skb_priv_t priv;
    size_t     len;   // amount of data in buf
    size_t     size;  // amount of mem alloc'ed
    uint8_t    buf[];
} skb_t;

typedef skb_t * skb_handle_t;

    // allocation and deallocation

/**
 * @brief Allocate a new `skb` (socket buffer) with the given size
 * 
 * Creates and initializes a socket buffer structure. Allocates a contiguous block
 * of memory large enough to hold both the `skb_t` metadata structure and the
 * requested buffer size.
 * 
 * @param  size         The size of the buffer to allocate
 * @return skb_handle_t A handle to the allocated skb, or nullptr on failure
 */

skb_handle_t skb_alloc(size_t size);

/**
 * @brief Free an `skb` (socket buffer)
 * 
 * Releases the memory allocated for a socket buffer and its associated data area.
 * The caller should ensure the skb is no longer in use before calling this function,
 * as the handle becomes invalid after this operation completes.
 * 
 * @param skb The handle to the `skb` to free
 */

void skb_free(skb_handle_t const skb);

    // data pointer manipulation

/**
 * @brief Reserve headroom for protocol headers
 * 
 * Reserves space at the beginning of the buffer for protocol headers to be added later.
 * This function moves both the data and tail pointers forward by the specified amount,
 * creating a gap between the head pointer and the data pointer. This reserved headroom
 * allows lower-layer protocol headers (such as Datalink layer headers) to be prepended
 * later using skb_push() without needing to shift existing data.
 * 
 * This function must be called immediately after skb_alloc() and before any data is
 * added to the buffer.
 * 
 * @param skb        The handle to the `skb` to reserve headroom for
 * @param header_len The length of the header to reserve
 */

void skb_reserve(skb_handle_t const skb, size_t const header_len);

/**
 * @brief Append data to the buffer and return pointer to write location
 * 
 * Expands the used data area at the end of the buffer by moving the tail pointer
 * forward. This function reserves space for new data and returns a pointer to where
 * that data should be written. The caller is responsible for actually writing the
 * data to the returned location.
 * 
 * This is typically used when appending payload data to a buffer that may already
 * contain headers. The function updates both the skb's length field and the tail
 * pointer to reflect the expanded data area. The assertion ensures that the requested
 * space doesn't exceed the buffer's capacity.
 * 
 * Example usage: After calling skb_reserve() to create headroom and skb_push() to
 * add headers, use skb_put() to reserve space for payload data, then copy the actual
 * payload to the returned pointer.
 * 
 * @param  skb           The handle to the `skb` to append data to
 * @param  user_data_len The length of the user data area to reserve
 * @return uint8_t*      A pointer to the start of the newly reserved data area
 */

uint8_t * skb_put(skb_handle_t const skb, size_t const user_data_len);

/**
 * @brief Reclaim previously allocated data space and return pointer to data
 * 
 * Shrinks the used data area at the end of the buffer by moving the tail pointer
 * backward (toward head). This is the inverse operation of skb_put() and is used
 * to "undo" or reclaim space that was previously reserved.
 * 
 * This function is useful when you've called skb_put() to reserve space but then
 * determine that you need less space than originally allocated. It reduces both
 * the skb's length field and the tail pointer, effectively shrinking the buffer's
 * used portion. The assertion ensures that the tail pointer doesn't move past the
 * head pointer.
 * 
 * After calling this function, the returned pointer points to the start of the
 * (now smaller) data area. This maintains the ability to access or process the
 * remaining data correctly.
 * 
 * @param  skb           The handle to the `skb` to reclaim space from
 * @param  user_data_adj The number of bytes to reclaim from the end
 * @return uint8_t*      A pointer to the start of the (adjusted) data area
 */

uint8_t * skb_call(skb_handle_t const skb, size_t const user_data_adj);

/**
 * @brief Prepend protocol header and return pointer to write location
 * 
 * Expands the used data area at the beginning of the buffer by moving the data
 * pointer backward (toward head). This function reserves space for a protocol
 * header and returns a pointer to where that header should be written.
 * 
 * This is used in a protocol stack to prepend headers as a packet moves down the
 * layers. For example, after appending payload with skb_put(), you would call
 * skb_push() to reserve space for the protocol header, write the header to the
 * returned pointer, and then pass the buffer to the next lower protocol layer.
 * 
 * The function updates both the skb's length field and the data pointer to reflect
 * the expanded data area. The assertion ensures that the data pointer doesn't move
 * past the head pointer, which would indicate buffer overflow.
 * 
 * Note: This is typically called AFTER skb_put() when building a packet, because
 * you usually add payload first, then prepend headers as you traverse down the
 * protocol stack.
 * 
 * @param  skb        The handle to the `skb` to prepend a header to
 * @param  header_len The length of the header to prepend
 * @return uint8_t*   A pointer to the start of the newly reserved header space
 */
  
uint8_t * skb_push(skb_handle_t const skb, size_t const header_len);

/**
 * @brief Remove protocol header and return pointer to user data
 * 
 * Shrinks the used data area at the beginning of the buffer by moving the data
 * pointer forward (toward tail). This is the inverse operation of skb_push() and
 * is used to strip away a processed header as a packet moves up the protocol stack.
 * 
 * This function is typically called when a received packet is being processed at
 * different protocol layers. Each layer examines its own header, extracts any needed
 * information, and then calls skb_pull() to remove that header before passing the
 * buffer to the next higher layer. This saves memory and CPU cycles by avoiding
 * the need to copy data.
 * 
 * After calling this function, the returned pointer points to the start of the
 * remaining data (now with the header stripped), which is ready for the next
 * protocol layer to process.
 * 
 * @param  skb        The handle to the `skb` to remove a header from
 * @param  header_len The length of the header to remove
 * @return uint8_t*   A pointer to the start of the remaining data
 */

uint8_t * skb_pull(skb_handle_t const skb, size_t const header_len);

/**
 * @brief Reset the buffer to its initial state
 * 
 * Reinitializes all internal pointers to their original positions, effectively
 * clearing the buffer for reuse. This is useful when you need to recycle an
 * allocated buffer instead of allocating a new one.
 * 
 * After calling this function, the buffer state matches what it would be
 * immediately after skb_alloc(), with head, data, and tail all pointing to
 * the start of the buffer, and end pointing to the end of the buffer capacity.
 * The length is reset to zero, indicating no data is in the buffer.
 * 
 * @param skb The handle to the `skb` to reset
 */

void skb_reset(skb_handle_t skb);

    // debugging

/**
 * @brief Format the buffer contents as a hex string
 * 
 * Converts the data portion of the buffer (from data pointer to tail pointer)
 * into a human-readable hex string. This is a debug utility for logging and
 * inspecting packet contents.
 * 
 * The function iterates through each byte in the buffer's data area and formats
 * it as a two-digit hex value followed by a space. The resulting string
 * is written to the provided buffer. If the output exceeds the buffer capacity,
 * the function stops writing to prevent overflow.
 * 
 * @param  tag      A tag string for logging purposes (typically used by ESP_LOGx)
 * @param  skb      The handle to the `skb` to format
 * @param  buf      A buffer where the formatted hex string will be written
 * @param  buf_size The size of the output buffer in bytes
 * @return size_t   The number of characters written to the buffer
 */

size_t skb_print(char const * const tag, skb_handle_t const skb, char * const buf, size_t const buf_size);

} // namespace opnpool
} // namespace esphome