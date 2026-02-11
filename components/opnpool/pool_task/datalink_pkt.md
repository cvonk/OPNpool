# Pentair Pool Protocol - Data Link Layer

This document describes the data link layer of the Pentair pool protocol as implemented by the OPNpool ESPHome component.

## Overview

The data link layer handles framing, addressing, and integrity checking for RS-485 communication with Pentair pool equipment. It supports three protocol variants that share the same physical layer but differ in frame structure.

## Protocol Types

| Protocol  | Enum Value | Description
|-----------|------------|------------
| `IC`      | 0x00       | IntelliChlor chlorinator protocol
| `A5_CTRL` | 0x01       | A5 controller protocol (EasyTouch/SunTouch)
| `A5_PUMP` | 0x02       | A5 pump protocol (IntelliFlo)
| `NONE`    | 0xFF       | No valid protocol detected

## Frame Structures

### A5 Protocol Frame (Controller & Pump)

| Offset | Field       | Size     | Value/Contents | Description
|--------|--------------|----------|----------------|------------
| 0      | Sync        | 1 byte   | `0xFF`         | Synchronization byte
| 1-3    | Preamble    | 3 bytes  | `00 FF A5`     | Packet start marker
| 4      | ver         | 1 byte   |                | Protocol version
| 5      | dst         | 1 byte   |                | Destination address
| 6      | src         | 1 byte   |                | Source address
| 7      | typ         | 1 byte   |                | Message type
| 8      | len         | 1 byte   |                | Data length
| 9-n    | Data        | variable |                | Payload (len bytes)
| n+1    | Checksum hi | 1 byte   |                | Checksum high byte
| n+2    | Checksum lo | 1 byte   |                | Checksum low byte


| Field    | Size | Description 
|----------|------|-------------
| FF       | 1    | Sync byte (0xFF)
| Preamble | 3    | `{0x00, 0xFF, 0xA5}` - protocol identifier
| ver      | 1    | Protocol version (typically 0x01)
| dst      | 1    | Destination address
| src      | 1    | Source address
| typ      | 1    | Message type
| len      | 1    | Data payload length
| data     | var  | Message payload (0-255 bytes)
| checksum | 2    | 16-bit checksum (big-endian)

### IC Protocol Frame (Chlorinator)

| Offset | Field     | Size     | Value
|--------|-----------|----------|------
| 0      | Sync      | 1 byte   | `0xFF`
| 1-2    | Preamble  | 2 bytes  | `10 02`
| 3      | dst       | 1 byte   |
| 4      | typ       | 1 byte   |
| 5-n    | Data      | variable |
| n+1    | Checksum  | 1 byte   |
| n+2    | Postamble | 2 bytes  | `10 03`

## Addressing Scheme

Addresses are 8-bit values with the following structure:

| Bits  | Field  | Size    | Description
|-------|--------|---------|------------
| 7-4   | Group  | 4 bits  | High nibble
| 3-0   | Device | 4 bits  | Low nibble

### Address Groups

| Group     | Value | Description 
|-----------|-------|-------------
| `ALL`     | 0x0_  | Broadcast to all devices
| `CTRL`    | 0x1_  | Controllers (0x10=SunTouch, 0x20=EasyTouch)
| `REMOTE`  | 0x2_  | Remotes (0x21=wired, 0x22=wireless/ScreenLogic)
| `CHLOR`   | 0x5_  | Chlorinators
| `PUMP`    | 0x6_  | IntelliFlo pumps (0x60-0x6F for pumps 0-15)
| `X09`     | 0x9_  | Unknown/reserved

### Device IDs

| ID          | Value | Description
|-------------|-------|-------------
| `PRIMARY`   | 0x00  | Primary device in group
| `SECONDARY` | 0x01  | Secondary device in group
| `REMOTE`    | 0x02  | Remote controller

## Message Types

### Controller Messages (`datalink_ctrl_typ_t`)

| Message        | Value | Description
|----------------|-------|------------
| `SET_ACK`      | 0x01  | Acknowledgment of a SET command
| `STATE_BCAST`  | 0x02  | Periodic state broadcast
| `CANCEL_DELAY` | 0x03  | Cancel delay timer
| `TIME_RESP`    | 0x05  | Time response
| `TIME_SET`     | 0x85  | Set time
| `TIME_REQ`     | 0xC5  | Request time
| `CIRCUIT_RESP` | 0x06  | Circuit state response
| `CIRCUIT_SET`  | 0x86  | Set circuit state
| `CIRCUIT_REQ`  | 0xC6  | Request circuit state
| `HEAT_RESP`    | 0x08  | Heat settings response
| `HEAT_SET`     | 0x88  | Set heat settings
| `HEAT_REQ`     | 0xC8  | Request heat settings
| `SCHED_RESP`   | 0x1E  | Schedule response
| `SCHED_SET`    | 0x9E  | Set schedule
| `SCHED_REQ`    | 0xDE  | Request schedule
| `LAYOUT_RESP`  | 0x21  | Layout response
| `LAYOUT_SET`   | 0xA1  | Set layout
| `LAYOUT_REQ`   | 0xE1  | Request layout
| `VERSION_RESP` | 0xFC  | Firmware version response
| `VERSION_REQ`  | 0xFD  | Request firmware version

**Message Type Encoding Pattern:**
- `0x0X` - Response messages
- `0x8X` - SET commands (request with data)
- `0xCX`/`0xDX`/`0xEX` - REQ commands (request without data)

### Pump Messages (`datalink_pump_typ_t`)

| Message  | Value | Description
|----------|-------|------------
| `REG`    | 0x01  | Register read/write
| `CTRL`   | 0x04  | Control command (local/remote)
| `MODE`   | 0x05  | Operating mode
| `RUN`    | 0x06  | Run/stop command
| `STATUS` | 0x07  | Status query/response

### Chlorinator Messages (`datalink_chlor_typ_t`)

| Message      | Value | Description
|--------------|-------|------------
| `STATUS_REQ`   | 0x00  | Ping request (keepalive)
| `STATUS_RESP`  | 0x01  | Ping response
| `MODEL_RESP`  | 0x03  | Name and salt level response
| `LEVEL_SET`  | 0x11  | Set chlorine output level
| `LEVEL_RESP` | 0x12  | Level and status response
| `MODEL_REQ`   | 0x14  | Request name and salt level

## Packet Structure

The `datalink_pkt_t` structure encapsulates a complete packet:

```cpp
struct datalink_pkt_t {
    datalink_prot_t    prot;      // Protocol type (IC, A5_CTRL, A5_PUMP)
    datalink_typ_t     typ;       // Message type (union of ctrl/pump/chlor)
    datalink_addr_t    src;       // Source address
    datalink_addr_t    dst;       // Destination address
    datalink_data_t *  data;      // Pointer to payload buffer
    size_t             data_len;  // Payload length
    skb_handle_t       skb;       // Socket buffer handle
};
```

## Checksum Calculation

* A5 Protocol: 16-bit checksum calculated from the last byte of the preamble (0xA5) through the end of the data payload
* IC Protocol: 8-bit checksum (lower byte of 16-bit calculation) from preamble through data payload

## Protocol Detection

The receiver distinguishes protocols by matching the preamble bytes:

1. Read bytes until sync (0xFF) is detected
2. Match subsequent bytes against known preambles:
   * `{0x10, 0x02}` → IC protocol
   * `{0x00, 0xFF, 0xA5}` → A5 protocol
3. For A5: distinguish CTRL vs PUMP based on source/destination address group (0x6_ = PUMP)

