#pragma once
#include <inttypes.h>
#include <pentair.h>

typedef struct {
    uint16_t active;
    uint16_t delay;
}  poolstate_circuits_t;

typedef struct {
    uint8_t temp;
    uint8_t setPoint;
    uint8_t heatSrc;
    bool heating;
} poolstate_heat_t;

typedef struct {
    uint8_t temp;
} poolstate_air_t;

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
    uint16_t rpm;
    uint8_t  err;
} poolstate_pump_t;

typedef struct {
    uint8_t pct;
    uint16_t salt;  // <2800 == low, <2600 == veryLow, > 4500 == high
    CHLORSTATE_t state;
} poolstate_chlor_t;

typedef struct poolstate_t {
    poolstate_time_t     time;
    poolstate_date_t     date;
    poolstate_heat_t     pool, spa;
    poolstate_air_t      air;
    poolstate_circuits_t circuits;
    poolstate_sched_t    sched[2];
    poolstate_pump_t     pump;
    poolstate_chlor_t    chlor;
} poolstate_t;


typedef struct heatState_t {
  struct {
    uint8_t setPoint;
    uint8_t heatSrc;
  } pool, spa;
} heatState_t;

void poolstate_init(void);
void poolstate_set(poolstate_t const * const state);
void poolstate_get(poolstate_t * const state);

