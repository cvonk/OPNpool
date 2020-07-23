#pragma once

#include <esp_system.h>
#include "../network/network.h"

typedef struct {
    uint16_t active;
    uint16_t delay;
}  poolstate_circuits_t;

typedef struct {
    uint8_t temp;
    uint8_t set_point;
    uint8_t heat_src;
    bool heating;
} poolstate_thermostat_t;

typedef struct {
    uint8_t temp;
} poolstate_temp_t;

typedef struct {
    uint8_t hour;
    uint8_t minute;
} poolstate_time_t ;

typedef struct {
    uint8_t day;
    uint8_t month;
    uint8_t year;
} poolstate_date_t;

typedef struct {
    uint8_t  circuit;
    uint16_t start;
    uint16_t stop;
} poolstate_sched_t;

typedef struct {
    bool     running;
    uint8_t  mode;
    uint8_t  status;
    uint16_t pwr;
    uint16_t gpm;
    uint16_t rpm;
    uint16_t pct;
    uint8_t  err;
    uint8_t  timer;
    poolstate_time_t time;
} poolstate_pump_t;

typedef struct {
    uint8_t pct;
    uint16_t salt;  // <2800 == low, <2600 == veryLow, > 4500 == high
    CHLORSTATE_t state;
} poolstate_chlor_t;

typedef struct poolstate_version_t {
    uint8_t major;
    uint8_t minor;
} poolstate_version_t;

typedef struct poolstate_tod_t {
    poolstate_date_t  date;
    poolstate_time_t  time;
}poolstate_tod_t;

typedef struct poolstate_t {
    poolstate_tod_t        tod;
    poolstate_version_t    version;
    poolstate_thermostat_t pool, spa;
    poolstate_temp_t       air, solar;
    poolstate_circuits_t   circuits;
    poolstate_sched_t      scheds[2];
    poolstate_pump_t       pump;
    poolstate_chlor_t      chlor;
} poolstate_t;

void poolstate_init(void);
void poolstate_set(poolstate_t const * const state);
void poolstate_get(poolstate_t * const state);

bool poolstate_receive_update(network_msg_t const * const msg, poolstate_t * const state);
