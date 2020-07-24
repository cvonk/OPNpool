#pragma once

#include <cJSON.h>

#include <esp_system.h>
#include "../network/network.h"
#include "../ipc/ipc.h"

#if !defined CONFIG_POOL_DBG_POOLSTATE
# define CONFIG_POOL_DBG_POOLSTATE (0)
#endif
#if !defined CONFIG_POOL_DBG_POOLSTATE_ONERROR
# define CONFIG_POOL_DBG_POOLSTATE_ONERROR (0)
#endif

/**
 * macro "magic" to get an enum and matching *_str functions (in *_str.c)
 **/

#define POOLSTATE_CHLOR_STATUS_MAP(XX) \
  XX(0, OK)           \
  XX(1, HIGH_SALT)    \
  XX(2, LOW_SALT)     \
  XX(3, VERYLOW_SALT) \
  XX(4, LOW_FLOW)

typedef enum {
#define XX(num, name) POOLSTATE_CHLOR_STATUS_##name = num,
  POOLSTATE_CHLOR_STATUS_MAP(XX)
#undef XX
} poolstate_chlor_status_t;

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
    poolstate_chlor_status_t status;
    mChlorNameStr name;
} poolstate_chlor_t;

typedef struct poolstate_version_t {
    uint8_t major;
    uint8_t minor;
} poolstate_version_t;

typedef struct poolstate_tod_t {
    poolstate_date_t  date;
    poolstate_time_t  time;
}poolstate_tod_t;

typedef struct poolstate_system_t {
    poolstate_tod_t        tod;
    poolstate_version_t    version;
} poolstate_system_t;

typedef struct poolstate_t {
    poolstate_system_t     system;
    poolstate_thermostat_t pool, spa;
    poolstate_temp_t       air, solar;
    poolstate_circuits_t   circuits;
    poolstate_sched_t      scheds[2];
    poolstate_pump_t       pump;
    poolstate_chlor_t      chlor;
} poolstate_t;

/* poolstate.c */
void poolstate_init(void);
void poolstate_set(poolstate_t const * const state);
void poolstate_get(poolstate_t * const state);

/* poolstate_rx.c */
bool poolstate_rx_update(network_msg_t const * const msg, poolstate_t * const state, ipc_t * const ipc_for_dbg);

/* cpool.c */
void cPool_AddPumpPrgToObject(cJSON * const obj, char const * const key, uint16_t const value);
void cPool_AddPumpCtrlToObject(cJSON * const obj, char const * const key, uint8_t const ctrl);
void cPool_AddPumpModeToObject(cJSON * const obj, char const * const key, uint8_t const mode);
void cPool_AddPumpRunningToObject(cJSON * const obj, char const * const key, bool const pump_state_running);
void cPool_AddPumpStatusToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);
void cPool_AddVersionToObject(cJSON * const obj, char const * const key, poolstate_version_t const * const version);
void cPool_AddDateToObject(cJSON * const obj, char const * const key, poolstate_date_t const * const date);
void cPool_AddTimeToObject(cJSON * const obj, char const * const key, poolstate_time_t const * const time);
void cPool_AddTodToObject(cJSON * const obj, char const * const key, poolstate_tod_t const * const state_tod);
void cPool_AddActiveCircuitsToObject(cJSON * const obj, char const * const key, uint16_t const active);
void cPool_AddThermostatToObject(cJSON * const obj, char const * const key, poolstate_thermostat_t const * const thermostat);
void cPool_AddThermostatSetToObject(cJSON * const obj, char const * const key, poolstate_thermostat_t const * const thermostat);
void cPool_AddThermostatStateToObject(cJSON * const obj, char const * const key, poolstate_thermostat_t const * const thermostat);
void cPool_AddTempToObject(cJSON * const obj, char const * const key, uint8_t const temp);
void cPool_AddSchedToObject(cJSON * const obj, char const * const key, uint16_t const start, uint16_t const stop);
void cPool_AddChlorRespToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);
void cPool_AddStateToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);

size_t poolstate_to_json(poolstate_t const * const state, char * const buf, size_t const buf_len);

/* poolstate_str.c */
const char * poolstate_chlor_state_str(poolstate_chlor_status_t const chlor_state);

