/**
 * @file skb.cpp
 * @brief Implementation of Linux sk_buff inspired socket buffer.
 *
 * @details
 * Originally based on http://www.keil.com/download/docs/200.asp by Dave Hylands.
 * Rewritten as C++ by Coert Vonk, 2015, 2019, 2026.
 *
 * @author Coert Vonk (@cvonk on GitHub)
 * @copyright Public Domain (CC0-1.0)
 * @license SPDX-License-Identifier: CC0-1.0
 */

#include <cassert>
#include <esp_system.h>
#include <esp_types.h>
#include <esphome/core/log.h>

#include "skb.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

namespace esphome {
namespace opnpool {

constexpr char TAG[] = "skb";

/**
 * @brief          Allocates a new socket buffer with the given size.
 *
 * @param[in] size The size of the buffer to allocate.
 * @return         Handle to the allocated skb, or nullptr on failure.
 */
skb_handle_t
skb_alloc(size_t const size)
{
    skb_t * const skb = static_cast<skb_t *>(calloc(1, sizeof(skb_t) + size));
    if (skb == nullptr) {
        ESP_LOGE(TAG, "skb_alloc: allocation failed");
        return nullptr;
    }
    skb->len       = 0;
    skb->size      = size;
    skb->priv.head =
    skb->priv.data =
    skb->priv.tail = skb->buf;
    skb->priv.end  = skb->buf + size;
    return skb;
}

/**
 * @brief         Frees a socket buffer.
 *
 * @param[in] skb Handle to the skb to free.
 */
void
skb_free(skb_handle_t const skb)
{
    free(skb);
}

/**
 * @brief                Reserves headroom for protocol headers.
 *
 * @param[in] skb        Handle to the skb to reserve headroom for.
 * @param[in] header_len The length of the header space to reserve.
 */
void
skb_reserve(skb_handle_t const skb, size_t const header_len)
{
    assert(skb->priv.head == skb->priv.tail);             // ensure no data has been added yet
    assert(skb->priv.tail + header_len <= skb->priv.end); // ensures buffer capacity is not exceeded
    skb->priv.data += header_len;
    skb->priv.tail += header_len;
}

/**
 * @brief                   Appends data to the buffer and returns pointer to write location.
 *
 * @param[in] skb           Handle to the skb to append data to.
 * @param[in] user_data_len The length of the user data area to reserve.
 * @return                  Pointer to the start of the newly reserved data area.
 */
uint8_t *
skb_put(skb_handle_t const skb, size_t const user_data_len)
{
    assert(skb->priv.tail + user_data_len <= skb->priv.end);
    uint8_t * const ret = skb->priv.tail;
    skb->len += user_data_len;
    skb->priv.tail += user_data_len;
    return ret;
}

/**
 * @brief                   Reclaims previously allocated data space from the end.
 *
 * @param[in] skb           Handle to the skb to reclaim space from.
 * @param[in] user_data_adj The number of bytes to reclaim from the end.
 * @return                  Pointer to the start of the (adjusted) data area.
 */
uint8_t *
skb_trim(skb_handle_t const skb, size_t const user_data_adj)
{
    assert(skb->priv.tail - user_data_adj >= skb->priv.head);
    skb->len -= user_data_adj;
    skb->priv.tail -= user_data_adj;
    return skb->priv.data;
}

/**
 * @brief                Prepends protocol header and returns pointer to write location.
 *
 * @param[in] skb        Handle to the skb to prepend a header to.
 * @param[in] header_len The length of the header to prepend.
 * @return               Pointer to the start of the newly reserved header space.
 */
uint8_t *
skb_push(skb_handle_t const skb, size_t const header_len)
{
    assert(skb->priv.data - header_len >= skb->priv.head);
    skb->len += header_len;
    return skb->priv.data -= header_len;
}

/**
 * @brief                Removes protocol header and returns pointer to remaining data.
 *
 * @param[in] skb        Handle to the skb to remove a header from.
 * @param[in] header_len The length of the header to remove.
 * @return               Pointer to the start of the remaining data.
 */
uint8_t *
skb_pull(skb_handle_t const skb, size_t const header_len)
{
    assert(skb->priv.data + header_len <= skb->priv.tail);  // ensure we don't pull beyond available data
    skb->len -= header_len;
    return skb->priv.data += header_len;
}

/**
 * @brief         Resets the buffer to its initial state.
 *
 * @param[in] skb Handle to the skb to reset.
 */
void
skb_reset(skb_handle_t const skb)
{
    skb->len       = 0;
    skb->priv.head =
    skb->priv.data =
    skb->priv.tail = skb->buf;
    skb->priv.end  = skb->buf + skb->size;
}

/**
 * @brief               Formats the buffer contents as a hex string.
 *
 * @param[in]  skb      Handle to the skb to format.
 * @param[out] buf      Buffer where the formatted hex string will be written.
 * @param[in]  buf_size The size of the output buffer in bytes.
 * @return              The number of characters written to the buffer.
 */
size_t
skb_print(skb_handle_t const skb, char * const buf, size_t const buf_size)
{
    size_t len = 0;
    for (size_t ii = 0; ii < skb->len; ii++) {
        len += snprintf(buf + len, buf_size - len, "%02x ", skb->priv.data[ii]);
    }
    return len;
}

}  // namespace opnpool
}  // namespace esphome
