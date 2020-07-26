#pragma once

#include <esp_system.h>
#include <cJSON.h>

#include "../network/network.h"
#include "../ipc/ipc.h"

#if !defined CONFIG_POOL_DBG_POOLSTATE
# define CONFIG_POOL_DBG_POOLSTATE (0)
#endif
#if !defined CONFIG_POOL_DBG_POOLSTATE_ONERROR
# define CONFIG_POOL_DBG_POOLSTATE_ONERROR (0)
#endif

/**
 * poolstate_system_t
 **/

typedef struct poolstate_time_t {
    uint8_t hour;
    uint8_t minute;
} poolstate_time_t ;

typedef struct poolstate_date_t {
    uint8_t day;
    uint8_t month;
    uint8_t year;
} poolstate_date_t;

typedef struct poolstate_tod_t {
    poolstate_date_t  date;
    poolstate_time_t  time;
} poolstate_tod_t;

typedef struct poolstate_version_t {
    uint8_t major;
    uint8_t minor;
} poolstate_version_t;

typedef struct poolstate_system_t {
    poolstate_tod_t        tod;
    poolstate_version_t    version;
} poolstate_system_t;

/**
 * poolstate_thermostats_t
 **/

#define POOLSTATE_THERMOSTAT_MAP(XX) \
  XX(0, POOL) \
  XX(1, SPA)  \
  XX(2, COUNT)

typedef enum {
#define XX(num, name) POOLSTATE_THERMOSTAT_##name = num,
  POOLSTATE_THERMOSTAT_MAP(XX)
#undef XX
} poolstate_thermostats_t;

typedef struct poolstate_sched_t {
    uint8_t  circuit;
    uint16_t start;
    uint16_t stop;
} poolstate_sched_t;

typedef struct poolstate_thermostat_t {
    uint8_t temp;
    uint8_t set_point;
    uint8_t heat_src;
    bool heating;
    poolstate_sched_t sched;
} poolstate_thermostat_t;

/**
 * poolstate_temp_t
 **/

#define POOLSTATE_TEMP_MAP(XX) \
  XX(0, AIR) \
  XX(1, SOLAR)  \
  XX(2, COUNT)

typedef enum {
#define XX(num, name) POOLSTATE_TEMP_##name = num,
  POOLSTATE_TEMP_MAP(XX)
#undef XX
} poolstate_temps_t;

typedef struct poolstate_temp_t {
    uint8_t temp;
} poolstate_temp_t;

/**
 * poolstate_circuits_t
 **/

typedef struct poolstate_circuits_t {
    bool     active[NETWORK_CIRCUIT_COUNT];
    uint8_t  delay;
} poolstate_circuits_t;

/**
 * poolstate_pump_t
 **/

typedef struct {
    poolstate_time_t time;
    uint8_t  mode;
    bool     running;
    uint8_t  status;
    uint16_t pwr;
    uint16_t gpm;
    uint16_t rpm;
    uint16_t pct;
    uint8_t  err;
    uint8_t  timer;
} poolstate_pump_t;

/**
 * poolstate_chlor_t
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

typedef struct poolstate_chlor_t {
    char                      name[sizeof(network_msg_chlor_name_str_t) + 1];
    uint8_t                   pct;
    uint16_t                  salt;  // <2800 == low, <2600 == veryLow, > 4500 == high
    poolstate_chlor_status_t  status;
} poolstate_chlor_t;

/**
 * all together now
 **/

typedef struct poolstate_t {
    poolstate_system_t      system;
    poolstate_temp_t        temps[POOLSTATE_TEMP_COUNT];
    poolstate_thermostat_t  thermostats[POOLSTATE_THERMOSTAT_COUNT];
    poolstate_circuits_t    circuits;
    poolstate_pump_t        pump;
    poolstate_chlor_t       chlor;
} poolstate_t;

/* poolstate.c */
void poolstate_init(void);
void poolstate_set(poolstate_t const * const state);
bool poolstate_get(poolstate_t * const state);

/* poolstate_rx.c */
bool poolstate_rx_update(network_msg_t const * const msg, poolstate_t * const state, ipc_t * const ipc_for_dbg);

/* poolstate_json.c */
void cJSON_AddTimeToObject(cJSON * const obj, char const * const key, poolstate_time_t const * const time);
void cJSON_AddDateToObject(cJSON * const obj, char const * const key, poolstate_date_t const * const date);
void cJSON_AddTodToObject(cJSON * const obj, char const * const key, poolstate_tod_t const * const tod);
void cJSON_AddVersionToObject(cJSON * const obj, char const * const key, poolstate_version_t const * const version);
void cJSON_AddSystemToObject(cJSON * const obj, char const * const key, poolstate_system_t const * const system);
void cJSON_AddSchedToObject(cJSON * const obj, char const * const key, poolstate_sched_t const * const sched);
void cJSON_AddThermostatToObject(cJSON * const obj, char const * const key, poolstate_thermostat_t const * const thermostat, bool const showTemp, bool showSp, bool const showSrc, bool const showHeating, bool const showSched);
void cJSON_AddThermostatsToObject(cJSON * const obj, char const * const key, poolstate_thermostat_t const * thermostats, bool const showTemp, bool showSp, bool const showSrc, bool const showHeating, bool const showSched);
void cJSON_AddTempToObject(cJSON * const obj, char const * const key, poolstate_temp_t const * const temp);
void cJSON_AddTempsToObject(cJSON * const obj, char const * const key, poolstate_temp_t const * temps);
void cJSON_AddActiveCircuitsToObject(cJSON * const obj, char const * const key, bool const * active);
void cJSON_AddCircuitsToObject(cJSON * const obj, char const * const key, poolstate_circuits_t const * const circuits);
void cJSON_AddStateToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);
void cJSON_AddPumpPrgToObject(cJSON * const obj, char const * const key, uint16_t const value);
void cJSON_AddPumpCtrlToObject(cJSON * const obj, char const * const key, uint8_t const ctrl);
void cJSON_AddPumpModeToObject(cJSON * const obj, char const * const key, uint8_t const mode);
void cJSON_AddPumpRunningToObject(cJSON * const obj, char const * const key, bool const running);
void cJSON_AddPumpStatusToObject(cJSON * const obj, char const * const key, poolstate_pump_t const * const pump);
void cJSON_AddPumpToObject(cJSON * const obj, char const * const key, poolstate_pump_t const * const pump);
void cJSON_AddChlorRespToObject(cJSON * const obj, char const * const key, poolstate_chlor_t const * const chlor);
void cJSON_AddChlorToObject(cJSON * const obj, char const * const key, poolstate_chlor_t const * const chlor);
size_t poolstate_to_json(poolstate_t const * const state, char * const buf, size_t const buf_len);

/* poolstate_str.c */
const char * poolstate_chlor_state_str(poolstate_chlor_status_t const chlor_state_id);
const char * poolstate_thermostat_str(poolstate_thermostats_t const thermostat_id);
const char * poolstate_temp_str(poolstate_temps_t const temp_id);
