# Pentair Pool Protocol Overview

This component implements communication with Pentair pool equipment over an RS-485 half-duplex serial bus. The protocol supports three device classes, each with its own protocol variant.

## Protocol Variants

| Variant     | Device Type                    | Preamble             | Checksum | Notes
|-------------|--------------------------------|----------------------|----------|------
| **A5_CTRL** | EasyTouch/SunTouch controllers | `{0x00, 0xFF, 0xA5}` | 16-bit   | Full addressing with source/destination
| **A5_PUMP** | IntelliFlo pumps               | `{0x00, 0xFF, 0xA5}` | 16-bit   | Same wire format as A5_CTRL
| **IC**      | IntelliChlor chlorinators      | `{0x10, 0x02}`       |  8-bit   | Simpler format with postamble `{0x10, 0x03}`

## Frame Structure

### A5 Protocol Frame

```
[0xFF] [preamble: 00 FF A5] [ver] [dst] [src] [typ] [len] [data...] [checksum_hi] [checksum_lo]
```

### IC Protocol Frame

```
[0xFF] [preamble: 10 02] [dst] [typ] [data...] [checksum] [postamble: 10 03]
```

## Addressing Scheme

Addresses are 8-bit values where:
- **High nibble**: Group address (device class)
- **Low nibble**: Device ID within the group

| Group  | Value | Description
|--------|-------|------------
| ALL    | 0x0_  | Broadcast
| CTRL   | 0x1_  | Controllers (0x10=SunTouch, 0x20=EasyTouch)
| REMOTE | 0x2_  | Remotes (0x21=wired, 0x22=wireless/ScreenLogic)
| CHLOR  | 0x5_  | Chlorinators
| PUMP   | 0x6_  | IntelliFlo pumps (0x60-0x6F for pumps 0-15)

## Message Types

### Controller Messages (`datalink_ctrl_typ_t`)

- State broadcasts (circuit status, temperatures, heat modes)
- Time/date synchronization
- Circuit on/off commands
- Heat setpoint control
- Schedule management
- Firmware version queries

### Pump Messages (`datalink_pump_typ_t`)

| Type   | Value | Description
|--------|-------|------------
| REG    | 0x1F  | Register read/write
| CTRL   | 0x04  | Control commands
| MODE   | 0x05  | Mode settings
| RUN    | 0x06  | Run status
| STATUS | 0x07  | Pump status queries

### Chlorinator Messages (`datalink_chlor_typ_t`)

| Type         | Value | Description
|--------------|-------|------------
| CONTROL_REQ  | 0x00  | Control/ping request
| CONTROL_RESP | 0x01  | Control/ping response
| MODEL_REQ    | 0x14  | Name request
| MODEL_RESP   | 0x03  | Name response (includes salt level)
| LEVEL_SET    | 0x11  | Set chlorine level (0-100%)
| LEVEL_SET10  | 0x15  | Set chlorine level × 10 (0.0-25.5%)
| LEVEL_RESP   | 0x12  | Level response (salt + error bits)
| ICHLOR_BCAST | 0x16  | iChlor broadcast (level + temp)

## Data Structures

The `network_msg.h` file uses packed structs with bit fields to exactly match the wire format. An X-Macro pattern (`NETWORK_MSG_TYP_LIST`) keeps the enum values, size table, and message info table synchronized.

### X-Macro Pattern

The `NETWORK_MSG_TYP_LIST` macro generates:
- `network_msg_typ_t` enum (all message types)
- `network_msg_typ_info[]` table (protocol, datalink type, size, direction)

Adding a new message type requires only one line in the macro:
```cpp
X(ENUM_NAME, sizeof(struct), is_to_pump, PROTOCOL, datalink_typ)
```

### Lookup Functions

Reverse lookups from datalink types to network message info:
```cpp
network_msg_typ_get_info(network_msg_typ_t typ);           // by enum
network_msg_typ_get_info(datalink_ctrl_typ_t ctrl_typ);    // controller messages
network_msg_typ_get_info(datalink_pump_typ_t pump_typ, bool is_to_pump);  // pump messages
network_msg_typ_get_info(datalink_chlor_typ_t chlor_typ);  // chlorinator messages
```

### Example: Controller State Broadcast

```cpp
struct network_ctrl_state_bcast_t {
    network_time_t       time;          // 0..1   hour, minute
    network_lo_hi_t      active;        // 2..3   bitmask for active circuits
    uint8_t              active_3;      // 4      bitmask for more active circuits
    uint8_t              active_4;      // 5      bitmask for more active circuits
    uint8_t              active_5;      // 6      bitmask for more active circuits
    uint8_t              unknown_07;    // 7
    uint8_t              unknown_08;    // 8
    network_ctrl_modes_t modes;         // 9      bitmask: service, temp_increase, freeze, timeout
    uint8_heat_status_t  heat_status;   // 10     bit2=POOL, bit3=SPA
    uint8_t              unknown_11;    // 11
    uint8_t              delay;         // 12     bitmask for delay status of circuits
    uint8_t              unknown_13;    // 13
    uint8_t              pool_temp;     // 14     water sensor 1 (POOL) [°F]
    uint8_t              spa_temp;      // 15     water sensor 2 (SPA) [°F]
    uint8_t              unknown_16;    // 16
    uint8_t              solar_temp_1;  // 17     solar sensor 1
    uint8_t              air_temp;      // 18     air sensor [°F]
    uint8_t              solar_temp_2;  // 19     solar sensor 2
    uint8_t              unknown_20;    // 20     maybe water sensor 3
    uint8_t              unknown_21;    // 21     maybe water sensor 4
    uint8_heat_src_t     heat_src;      // 22     lower nibble=POOL, upper nibble=SPA
    uint8_t              heat_src_2;    // 23     lower nibble=body3, upper nibble=body4
    uint8_t              unknown_24;    // 24
    uint8_t              unknown_25;    // 25
    uint8_t              unknown_26;    // 26
    network_hi_lo_t      ocp_id;        // 27..28 outdoor control panel ID
} PACK8;
```

### Example: Pump Status Response

```cpp
struct network_pump_status_resp_t {
    network_pump_running_t  running;    // 0      0x0A=on, 0x04=off
    network_pump_run_mode_t mode;       // 1      Filter, Manual, Backwash, etc.
    network_pump_state_t    state;      // 2      OK, PRIMING, RUNNING, SYS_PRIMING
    network_hi_lo_t         power;      // 3..4   [Watt]
    network_hi_lo_t         speed;      // 5..6   [rpm]
    uint8_t                 flow;       // 7      [gal/min]
    uint8_t                 level;      // 8      [%]
    uint8_t                 unknown;    // 9
    uint8_t                 error;      // 10     error code
    network_time_t          remaining;  // 11..12 time remaining (or status bits)
    network_time_t          clock;      // 13..14 pump clock
} PACK8;
```

### Example: Heat Control

```cpp
struct network_ctrl_heat_resp_t {
    uint8_t          pool_temp;        // 0   water sensor 1 (POOL) [°F]
    uint8_t          spa_temp;         // 1   water sensor 2 (SPA) [°F]
    uint8_t          air_temp;         // 2   air sensor [°F]
    uint8_t          pool_set_point;   // 3   body 1 set-point (POOL) [°F]
    uint8_t          spa_set_point;    // 4   body 2 set-point (SPA) [°F]
    uint8_heat_src_t heat_src;         // 5   bits 0-3=POOL, bits 4-7=SPA
    // ... bodies 3/4 for multi-body systems
} PACK8;

enum class network_heat_src_t : uint8_t {
    NONE, Heat, SolarPreferred, Solar
};
```

### Example: Chlorinator Level Response

```cpp
struct network_chlor_level_resp_t {
    uint8_t salt;   // parts per million / 50
    uint8_t error;  // bit0=low_flow, bit1=low_salt, bit2=high_salt,
                    // bit4=clean_cell, bit6=cold, bit7=OK
} PACK8;
```

## Communication Flow

1. Reception (datalink_rx.cpp): Strips preamble/postamble, validates checksum, extracts header and payload
2. Transmission (datalink_tx.cpp): Adds protocol-specific header, calculates checksum, queues for RS-485 transmission

The implementation uses a socket buffer (skb) abstraction for zero-copy packet handling, with `skb_push()` to prepend headers and `skb_put()` to append trailers.

## Controller Modes

The `network_ctrl_modes_t` struct decodes the mode byte:
- Bit 0: Service mode
- Bit 2: Temperature increase mode
- Bit 3: Freeze protection mode
- Bit 4: Timeout mode

## Circuit Enumeration

```cpp
enum class network_pool_circuit_t : uint8_t {
    SPA = 0, AUX1, AUX2, AUX3, FEATURE1, POOL, FEATURE2, FEATURE3, FEATURE4
};
```
