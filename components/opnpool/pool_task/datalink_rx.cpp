/**
 * @file datalink_rx.cpp
 * @brief Data Link layer: bytes from the RS485 transceiver to data packets
 *
 * @details
 * This file implements the data link layer receiver for the OPNpool component,
 * responsible for converting raw bytes from the RS485 transceiver into structured data
 * packets. It uses a state machine to detect protocol preambles, read packet headers,
 * data, and tails, and verify checksums for both A5 and IC protocols. The implementation
 * manages protocol-specific framing, handles checksum validation, and allocates socket buffers
 * for incoming packets. This layer ensures reliable and robust extraction of protocol
 * packets from the RS485 byte stream, providing validated data to higher-level network
 * processing in the OPNpool interface.
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
#include <esp_err.h>
#include <esphome/core/log.h>

#include "rs485.h"
#include "network.h"
#include "skb.h"
#include "datalink.h"
#include "datalink_pkt.h"
#include "network_msg.h"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused-parameter"

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

namespace esphome {
namespace opnpool {

constexpr char TAG[] = "datalink_rx";

    // protocol preamble matching state
struct proto_info_t {
    uint8_t const * const  preamble;
    uint8_t const          len;
    datalink_prot_t const  prot;
    uint8_t                idx;
};

static proto_info_t _proto_descr[] = {
    {
        .preamble = datalink_preamble_ic,
        .len = sizeof(datalink_preamble_ic),
        .prot = datalink_prot_t::IC,
        .idx = 0
    },
    {
        .preamble = datalink_preamble_a5,
        .len = sizeof(datalink_preamble_a5),
        .prot = datalink_prot_t::A5_CTRL,  // distinction between A5_CTRL and A5_PUMP is based on src/dst in hdr
        .idx = 0
    },
};

    // size lookup table for message types
    // MUST MATCH enum datalink_chlor_typ_t in datalink_pkt.h
inline constexpr size_t datalink_chlor_typ_sizes[] = {
    sizeof(network_chlor_control_req_t),    // 0x00 CONTROL_REQ
    sizeof(network_chlor_control_resp_t),   // 0x01 CONTROL_RESP
    0,                                      // 0x02 UNKNOWN_02
    sizeof(network_chlor_model_resp_t),     // 0x03 MODEL_RESP
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x04..0x10 UNKNOWN_04..10
    sizeof(network_chlor_level_set_t),      // 0x11 LEVEL_SET
    sizeof(network_chlor_level_resp_t),     // 0x12 LEVEL_RESP
    0,                                      // 0x13 ICHLOR_PING
    sizeof(network_chlor_model_req_t),      // 0x14 MODEL_REQ
    sizeof(network_chlor_level10_set_t),    // 0x15 LEVEL_SET10
    sizeof(network_chlor_ichlor_bcast_t),   // 0x16 ICHLOR_BCAST
};
static_assert(enum_count<datalink_chlor_typ_t>() == ARRAY_SIZE(datalink_chlor_typ_sizes));

    // state machine states for packet reception
enum state_t {
    STATE_FIND_PREAMBLE,
    STATE_READ_HEAD,
    STATE_READ_DATA,
    STATE_READ_TAIL,
    STATE_CHECK_CHECKSUM,
    STATE_DONE,
};

    // temporary storage for packet header/tail during reception
struct local_data_t {
    size_t             head_len;
    size_t             tail_len;
    datalink_head_t *  head;
    datalink_tail_t *  tail;
    bool               checksum_ok;
};

/**
 * @brief Reset the preamble match state for all supported protocols.
 *
 * Resets the internal state for all protocol preamble matchers, preparing them to
 * detect the start of a new packet in the RS-485 byte stream. Called at the beginning
 * of packet reception and after failed matches.
 */
static void
_preamble_reset()
{
    proto_info_t * info = _proto_descr;

    for (uint_least8_t ii = 0; ii < ARRAY_SIZE(_proto_descr); ii++, info++) {
        info->idx = 0;
    }
}

/**
 * @brief Check if the preamble for a protocol is complete given the next byte.
 *
 * Examines the next byte from the RS-485 stream to determine if it matches the expected
 * protocol preamble sequence. Advances the match index and sets a flag if the byte is
 * part of the preamble. Returns true if the full preamble is matched.
 *
 * @param[in,out] pi               Protocol info structure (idx is updated).
 * @param[in]     b                Next byte from the stream.
 * @param[out]    part_of_preamble Set true if b matches part of the preamble.
 * @return                         True if preamble is complete, false otherwise.
 */
[[nodiscard]] static bool
_preamble_complete(proto_info_t * const pi, uint8_t const b, bool * part_of_preamble)
{
    if (b == pi->preamble[pi->idx]) {
        *part_of_preamble = true;
        pi->idx++;
        if (pi->idx == pi->len) {
            return true;
        }
    } else {
        *part_of_preamble = false;
    }
    return false;
}

/**
 * @brief Waits until a valid A5/IC protocol preamble is received (or times-out).
 *
 * Reads bytes from the RS-485 interface until a valid preamble for either the A5 or IC
 * protocol is detected. Updates the packet structure with the detected protocol type and
 * stores the received preamble bytes in the local header buffer. Also sets header/tail
 * lengths for the detected protocol.
 *
 * @param[in]     rs485 RS485 handle.
 * @param[in,out] local Local state for header/tail.
 * @param[out]    pkt   Packet structure to update.
 * @return              ESP_OK if preamble found, ESP_FAIL otherwise.
 */
[[nodiscard]] static esp_err_t
_find_preamble(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt)
{
    uint8_t len = 0;
    uint8_t const buf_size = 40;
    char dbg[buf_size];

    uint8_t byt;
    while (rs485->read_bytes(&byt, 1) == 1) {
        if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
            len += snprintf(dbg + len, buf_size - len, " %02X", byt);
        }
        bool part_of_preamble = false;
        proto_info_t * info = _proto_descr;
        
        for (uint_least8_t ii = 0; !part_of_preamble && ii < ARRAY_SIZE(_proto_descr); ii++, info++) {
            if (_preamble_complete(info, byt, &part_of_preamble)) {
                ESP_LOGV(TAG, "%s (preamble)", dbg);
                pkt->prot = info->prot;
                uint8_t * preamble = nullptr;
                switch (pkt->prot) {
                    case datalink_prot_t::A5_CTRL:
                    case datalink_prot_t::A5_PUMP:
                        // add to pkt just in case we want to retransmit it
                        local->head->a5.ff = 0xFF;
                        preamble = local->head->a5.preamble;
                        local->head_len = sizeof(datalink_head_a5_t) ;
                        local->tail_len = sizeof(datalink_tail_a5_t) ;
                        break;
                    case datalink_prot_t::IC:
                        preamble = local->head->ic.preamble;
                        local->head_len = sizeof(datalink_head_ic_t) ;
                        local->tail_len = sizeof(datalink_tail_ic_t) ;
                        break;
                    default:
                        return ESP_FAIL;
                }
                for (uint_least8_t jj = 0; jj < info->len; jj++) {
                    preamble[jj] = info->preamble[jj];
                }
                _preamble_reset();
                return ESP_OK;
            }
        }

        if (!part_of_preamble) {  // could be the beginning of the next
            _preamble_reset();
            proto_info_t * info = _proto_descr;

            for (uint_least8_t ii = 0; ii < ARRAY_SIZE(_proto_descr); ii++, info++) {
                (void)_preamble_complete(info, byt, &part_of_preamble);
            }
        }
    }
    return ESP_FAIL;
}

/**
 * @brief            Returns the length of the IC network message for a given type.
 *
 * Looks up the expected length of the IC protocol network message for the given type.
 *
 * @param[in] ic_typ The IC message type (as uint8_t/datalink_chlor_typ_t).
 * @return           The size of the corresponding network message struct, or 0 if unknown.
 */
[[nodiscard]] static uint8_t
_network_ic_len(uint8_t const ic_typ)
{
    if (ic_typ < ARRAY_SIZE(datalink_chlor_typ_sizes)) {
        return datalink_chlor_typ_sizes[ic_typ];
    }
    ESP_LOGW(TAG, "Unknown IC message type: %02X", ic_typ);
    return 0;
}

/**
 * @brief Reads an A5/IC protocol header (or times-out).
 *
 * Reads the header portion of a detected A5 or IC protocol packet from the RS-485 bus.
 * Populates the packet structure with type, source, destination, and data length fields.
 *
 * @param[in]     rs485 RS485 handle.
 * @param[in,out] local Local state for header/tail.
 * @param[out]    pkt   Packet structure to update.
 * @return              ESP_OK if header read, ESP_FAIL otherwise.
 */
[[nodiscard]] static esp_err_t
_read_head(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt)
{
    switch (pkt->prot) {
        case datalink_prot_t::A5_CTRL:
        case datalink_prot_t::A5_PUMP: {
            datalink_hdr_a5_t * const hdr = &local->head->a5.hdr;

            if (rs485->read_bytes((uint8_t *) hdr, sizeof(datalink_hdr_a5_t)) == sizeof(datalink_hdr_a5_t)) {

                ESP_LOGV(TAG, " %02X %02X %02X %02X %02X (header)", hdr->ver, hdr->dst.addr, hdr->src.addr, hdr->typ, hdr->len);

                if (hdr->len > DATALINK_MAX_DATA_SIZE) {
                    return ESP_FAIL;  // pkt length exceeds what we have planned for
                }
                if ( hdr->src.is_pump() || hdr->dst.is_pump() ) {
                    pkt->prot = datalink_prot_t::A5_PUMP;
                }
                pkt->typ.raw  = hdr->typ;
                pkt->src      = hdr->src;
                pkt->dst      = hdr->dst;
                pkt->data_len = hdr->len;
                if (pkt->data_len > sizeof(network_data_a5_t)) {
                    return ESP_FAIL;
                }
                return ESP_OK;
            }
            break;
        }
        case datalink_prot_t::IC: {
            datalink_hdr_ic_t * const hdr = &local->head->ic.hdr;
            
            if (rs485->read_bytes((uint8_t *) hdr, sizeof(datalink_hdr_ic_t)) == sizeof(datalink_hdr_ic_t)) {
                ESP_LOGV(TAG, " %02X %02X (header)", hdr->dst.addr, hdr->typ);

                pkt->typ.raw  = hdr->typ;
                pkt->src      = datalink_addr_t::unknown();
                pkt->dst      = hdr->dst;
                pkt->data_len = _network_ic_len(hdr->typ);
                return ESP_OK;
            }
            break;
        }
        default:
            break;
    }
    ESP_LOGW(TAG, "unsupported pkt->prot 0x%02X", static_cast<uint8_t>(pkt->prot));
    return ESP_FAIL;
}

/**
 * @brief Reads the data payload of a previously detected A5 or IC protocol packet.
 *
 * Reads the data section from the RS-485 bus and stores it in the packet's data buffer.
 * Called after the header has been successfully read.
 *
 * @param[in]     rs485 RS485 handle.
 * @param[in]     local Local state for header/tail (unused).
 * @param[in,out] pkt   Packet structure to update with data.
 * @return              ESP_OK if data read, ESP_FAIL otherwise.
 */
[[nodiscard]] static esp_err_t
_read_data(rs485_handle_t const rs485, [[maybe_unused]] local_data_t * const local, datalink_pkt_t * const pkt)
{
    constexpr uint8_t buf_size = 100;
    char buf[buf_size]; *buf = '\0';

    if (rs485->read_bytes((uint8_t *) pkt->data, pkt->data_len) == pkt->data_len) {
        if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE) {
            uint8_t len = 0;
            for (uint_least8_t ii = 0; ii < pkt->data_len; ii++) {
                len += snprintf(buf + len, buf_size - len, " %02X", pkt->data[ii]);
            }
            ESP_LOGV(TAG, "%s (data)", buf);
        }
        return ESP_OK;
    }
    return ESP_FAIL;
}

/**
 * @brief Reads the tail (checksum and postamble) of a previously detected A5 or IC protocol packet.
 *
 * Reads the tail section from the RS-485 bus, which contains the checksum and, for IC protocol,
 * the postamble. Stores the received bytes in the local tail buffer.
 *
 * @param[in]     rs485 RS485 handle.
 * @param[in,out] local Local state for header/tail.
 * @param[in]     pkt   Packet structure with protocol info.
 * @return              ESP_OK if tail read, ESP_FAIL otherwise.
 */
[[nodiscard]] static esp_err_t
_read_tail(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt)
{
    switch (pkt->prot) {
        case datalink_prot_t::A5_CTRL:
        case datalink_prot_t::A5_PUMP: {
            uint8_t * const checksum = local->tail->a5.checksum;
            if (rs485->read_bytes(checksum, sizeof(datalink_tail_a5_t)) == sizeof(datalink_tail_a5_t)) {
                ESP_LOGV(TAG, " %03X (checksum)", (uint16_t)checksum[0] << 8 | checksum[1]);
                return ESP_OK;
            }
            break;
        }
        case datalink_prot_t::IC: {
            uint8_t * const checksum = local->tail->ic.checksum;
            uint8_t * const postamble = local->tail->ic.postamble;
            if (rs485->read_bytes(checksum, sizeof(datalink_tail_ic_t)) == sizeof(datalink_tail_ic_t)) {
                ESP_LOGV(TAG, " %02X (checksum)", checksum[0]);
                ESP_LOGV(TAG, " %02X %02X (postamble)", postamble[0], postamble[1]);
                return ESP_OK;
            }
            break;
        }
        default:
            break;
    }
    
    ESP_LOGW(TAG, "unsupported pkt->prot 0x%02X !", static_cast<uint8_t>(pkt->prot));
    return ESP_FAIL;
}

/**
 * @brief Check the checksum of the received packet.
 *
 * Verifies the checksum of the received packet by comparing the received value with the
 * calculated checksum over the packet's contents. Updates the local checksum_ok status.
 *
 * @param[in]     rs485 RS485 handle (unused).
 * @param[in,out] local Local state for header/tail (checksum_ok is updated).
 * @param[in]     pkt   Packet structure to check.
 * @return              ESP_OK if checksum matches, ESP_FAIL otherwise.
 */
[[nodiscard]] static esp_err_t
_check_checksum([[maybe_unused]] rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt)
{
    struct {uint16_t rx, calc;} checksum;

    switch (pkt->prot) {
        case datalink_prot_t::A5_CTRL:
        case datalink_prot_t::A5_PUMP: {
            checksum.rx = (uint16_t)local->tail->a5.checksum[0] << 8 | local->tail->a5.checksum[1];
            uint8_t * const start = &local->head->a5.preamble[sizeof(datalink_preamble_a5_t) - 1];  // starting at the last byte of the preamble
            uint8_t * const stop = pkt->data + pkt->data_len;
            checksum.calc = datalink_calc_checksum(start, stop);
            break;
        }
        case datalink_prot_t::IC: {
            checksum.rx = local->tail->ic.checksum[0];
            uint8_t * const start = local->head->ic.preamble;  // starting at the first byte of the preamble
            uint8_t * const stop = pkt->data + pkt->data_len;
            checksum.calc = datalink_calc_checksum(start, stop) & 0xFF;
            break;
        }
        default:
            return ESP_FAIL;
    }

    local->checksum_ok = checksum.rx == checksum.calc;
    if (local->checksum_ok) {
        return ESP_OK;
    }

    ESP_LOGW(TAG, "checksum err (rx=0x%03x calc=0x%03x)", checksum.rx, checksum.calc);
    return ESP_FAIL;
}

    // state machine function signature
using state_fnc_t = esp_err_t (*)(rs485_handle_t const rs485, local_data_t * const local, datalink_pkt_t * const pkt);

    // state machine transition table entry
struct state_transition_t {
    state_t     const state;
    state_fnc_t const fnc;
    state_t     const on_ok;
    state_t     const on_err;
};

static state_transition_t state_transitions[] = {
    { STATE_FIND_PREAMBLE,  _find_preamble,   STATE_READ_HEAD,      STATE_FIND_PREAMBLE },
    { STATE_READ_HEAD,      _read_head,       STATE_READ_DATA,      STATE_FIND_PREAMBLE },
    { STATE_READ_DATA,      _read_data,       STATE_READ_TAIL,      STATE_FIND_PREAMBLE },
    { STATE_READ_TAIL,      _read_tail,       STATE_CHECK_CHECKSUM, STATE_FIND_PREAMBLE },
    { STATE_CHECK_CHECKSUM, _check_checksum,  STATE_DONE,           STATE_FIND_PREAMBLE },
};

/**
 * @brief Receive a protocol packet from the RS-485 bus using a state machine.
 *
 * This function implements the main receive loop for the data link layer. It uses a state
 * machine (see `state_transitions[]`) to detect protocol preambles, read packet headers,
 * payloads, and tails, and verify checksums for supported protocols (A5 and IC). The
 * function allocates a socket buffer, extracts and validates the packet, and returns the
 * result to the caller.
 *
 * Called from `pool_task` to process incoming RS-485 data and convert it into structured
 * packets for higher-level network processing.
 *
 * @param[in]  rs485 RS485 handle for reading bytes from the bus.
 * @param[out] pkt   Pointer to a packet structure to fill with received data.
 * @return           ESP_OK if a valid packet is received and checksum matches, ESP_FAIL otherwise.
 */
esp_err_t
datalink_rx_pkt(rs485_handle_t const rs485, datalink_pkt_t * const pkt)
{
    state_t state = STATE_FIND_PREAMBLE;
    pkt->skb = skb_alloc(DATALINK_MAX_HEAD_SIZE + DATALINK_MAX_DATA_SIZE + DATALINK_MAX_TAIL_SIZE);
    if (!pkt->skb) {
        ESP_LOGW(TAG, "Failed to allocate socket buffer");
        return ESP_FAIL;
    }
    local_data_t local;
    local.head = (datalink_head_t *) skb_put(pkt->skb, DATALINK_MAX_HEAD_SIZE);

    while (true) {
        state_transition_t * transition = state_transitions;
        for (uint_least8_t ii = 0; ii < ARRAY_SIZE(state_transitions); ii++, transition++) {
            if (state == transition->state) {

                    // calls the registered function for the current state. it will store
                    // head/tail in `local` and update `pkt`

                bool const ok = transition->fnc(rs485, &local, pkt) == ESP_OK;

                    // find the new state

                state_t const new_state = ok ? transition->on_ok : transition->on_err;

                    // claim socket buffers to store the bytes received

                switch (new_state) {
                    case STATE_FIND_PREAMBLE:
                        skb_reset(pkt->skb);
                        local.head = (datalink_head_t *) skb_put(pkt->skb, DATALINK_MAX_HEAD_SIZE);
                        break;
                    case STATE_READ_HEAD:
                        skb_trim(pkt->skb, DATALINK_MAX_HEAD_SIZE - local.head_len);  // release unused bytes
                        break;
                    case STATE_READ_DATA:
                        pkt->data = (datalink_data_t *) skb_put(pkt->skb, pkt->data_len);
                        break;
                    case STATE_READ_TAIL:
                        local.tail = (datalink_tail_t *) skb_put(pkt->skb, local.tail_len);
                        break;
                    case STATE_CHECK_CHECKSUM:
                        break;
                    case STATE_DONE:
                        return ESP_OK;
                }
                state = new_state;
            }
        }
    }
}

} // namespace opnpool
} // namespace esphome