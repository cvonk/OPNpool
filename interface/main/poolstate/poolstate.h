#pragma once

#include <esp_system.h>
#include <cJSON.h>

#include "../network/network.h"
#include "../ipc/ipc.h"

/**
 * poolstate_system_t
 **/

typedef struct poolstate_time_t {
    uint8_t hour;
    uint8_t minute;
} poolstate_time_t;

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
    poolstate_tod_t     tod;
    poolstate_version_t version;
} poolstate_system_t;

#define POOLSTATE_ELEM_SYSTEM_TYP_MAP(XX) \
  XX(0, TIME) \
  XX(1, VERSION)

typedef enum {
#define XX(num, name) POOLSTATE_ELEM_SYSTEM_TYP_##name = num,
  POOLSTATE_ELEM_SYSTEM_TYP_MAP(XX)
#undef XX
} poolstate_elem_system_typ_t;

/**
 * poolstate_thermo_typ_t
 **/

#define POOLSTATE_THERMO_TYP_MAP(XX) \
  XX(0, POOL) \
  XX(1, SPA)  \
  XX(2, COUNT)

typedef enum {
#define XX(num, name) POOLSTATE_THERMO_TYP_##name = num,
  POOLSTATE_THERMO_TYP_MAP(XX)
#undef XX
} poolstate_thermo_typ_t;

typedef struct poolstate_thermo_t {
    uint8_t           temp;
    uint8_t           set_point;
    uint8_t           heat_src;
    bool              heating;
} poolstate_thermo_t;

#define POOLSTATE_ELEM_THERMO_TYP_MAP(XX) \
  XX(0, TEMP) \
  XX(1, SET_POINT) \
  XX(2, HEAT_SRC) \
  XX(3, HEATING)

typedef enum {
#define XX(num, name) POOLSTATE_ELEM_THERMO_TYP_##name = num,
  POOLSTATE_ELEM_THERMO_TYP_MAP(XX)
#undef XX
} poolstate_elem_thermo_typ_t;

/**
 * poolstate_sched_t
 */

#define POOLSTATE_ELEM_SCHED_TYP_MAP(XX) \
  XX(0, CIRCUIT) \
  XX(1, START) \
  XX(2, STOP)

typedef enum {
#define XX(num, name) POOLSTATE_ELEM_SCHED_TYP_##name = num,
  POOLSTATE_ELEM_SCHED_TYP_MAP(XX)
#undef XX
} poolstate_elem_sched_typ_t;

typedef struct poolstate_sched_t {
    network_circuit_t circuit;
    uint16_t          start;
    uint16_t          stop;
} poolstate_sched_t;

#define POOLSTATE_SCHED_TYP_COUNT (2)

/**
 * poolstate_temp_t
 **/

#define POOLSTATE_TEMP_TYP_MAP(XX) \
  XX(0, AIR) \
  XX(1, SOLAR)  \
  XX(2, COUNT)

typedef enum {
#define XX(num, name) POOLSTATE_TEMP_TYP_##name = num,
  POOLSTATE_TEMP_TYP_MAP(XX)
#undef XX
} poolstate_temp_typ_t;

typedef struct poolstate_temp_t {
    uint8_t temp;
} poolstate_temp_t;

#define POOLSTATE_ELEM_TEMP_TYP_MAP(XX) \
  XX(0, TEMP) \

typedef enum {
#define XX(num, name) POOLSTATE_ELEM_TEMP_TYP_##name = num,
  POOLSTATE_ELEM_TEMP_TYP_MAP(XX)
#undef XX
} poolstate_elem_temp_typ_t;

/**
 * poolstate_mode_t
 **/

typedef struct poolstate_modes_t {
    bool     set[NETWORK_MODE_COUNT];
} poolstate_modes_t;

/**
 * poolstate_circuits_t
 **/

typedef struct poolstate_circuits_t {
    bool     active[NETWORK_CIRCUIT_COUNT];
    bool     delay[NETWORK_CIRCUIT_COUNT];
} poolstate_circuits_t;

#define POOLSTATE_ELEM_CIRCUITS_TYP_MAP(XX) \
  XX(0, ACTIVE) \
  XX(1, DELAY)

typedef enum {
#define XX(num, name) POOLSTATE_ELEM_CIRCUITS_TYP_##name = num,
  POOLSTATE_ELEM_CIRCUITS_TYP_MAP(XX)
#undef XX
} poolstate_elem_circuits_typ_t;

/**
 * poolstate_pump_t
 **/

typedef struct {
    poolstate_time_t time;
    uint8_t  mode;
    bool     running;
    uint8_t  state;
    uint16_t pwr;
    uint16_t gpm;
    uint16_t rpm;
    uint16_t pct;
    uint8_t  err;
    uint8_t  timer;
} poolstate_pump_t;

#define POOLSTATE_ELEM_PUMP_TYP_MAP(XX) \
  XX(0, TIME) \
  XX(1, MODE) \
  XX(2, RUNNING) \
  XX(3, STATE) \
  XX(4, PWR) \
  XX(5, GPM) \
  XX(6, RPM) \
  XX(7, PCT) \
  XX(8, ERR) \
  XX(9, TIMER)

typedef enum {
#define XX(num, name) POOLSTATE_ELEM_PUMP_TYP_##name = num,
  POOLSTATE_ELEM_PUMP_TYP_MAP(XX)
#undef XX
} poolstate_elem_pump_typ_t;

/**
 * poolstate_chlor_t
 **/

#define POOLSTATE_CHLOR_STATUS_MAP(XX) \
  XX(0, OK)       \
  XX(1, LOW_FLOW) \
  XX(2, OTHER)

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

#define POOLSTATE_ELEM_CHLOR_TYP_MAP(XX) \
  XX(0, NAME) \
  XX(1, PCT) \
  XX(2, SALT) \
  XX(3, STATUS)

typedef enum {
#define XX(num, name) POOLSTATE_ELEM_CHLOR_TYP_##name = num,
  POOLSTATE_ELEM_CHLOR_TYP_MAP(XX)
#undef XX
} poolstate_elem_chlor_typ_t;

/**
 * all together now
 **/

typedef struct poolstate_t {
    poolstate_system_t    system;
    poolstate_temp_t      temps[POOLSTATE_TEMP_TYP_COUNT];
    poolstate_thermo_t    thermos[POOLSTATE_THERMO_TYP_COUNT];
    poolstate_sched_t     scheds[POOLSTATE_SCHED_TYP_COUNT];
    poolstate_modes_t     modes;
    poolstate_circuits_t  circuits;
    poolstate_pump_t      pump;
    poolstate_chlor_t     chlor;
} poolstate_t;

#define POOLSTATE_ELEM_TYP_MAP(XX) \
  XX(0x00, SYSTEM) \
  XX(0x01, TEMP) \
  XX(0x02, THERMO) \
  XX(0x03, SCHED) \
  XX(0x04, CIRCUITS) \
  XX(0x05, PUMP) \
  XX(0x06, CHLOR) \
  XX(0x07, MODES) \
  XX(0x08, ALL)

typedef enum {
#define XX(num, name) POOLSTATE_ELEM_TYP_##name = num,
  POOLSTATE_ELEM_TYP_MAP(XX)
#undef XX
} poolstate_elem_typ_t;

/* poolstate.c */
void poolstate_init(void);
void poolstate_set(poolstate_t const * const state);
esp_err_t poolstate_get(poolstate_t * const state);

/* poolstate_rx.c */
esp_err_t poolstate_rx_update(network_msg_t const * const msg, poolstate_t * const state, ipc_t const * const ipc_for_dbg);

/* poolstate_json.c */
void cJSON_AddTimeToObject(cJSON * const obj, char const * const key, poolstate_time_t const * const time);
void cJSON_AddDateToObject(cJSON * const obj, char const * const key, poolstate_date_t const * const date);
void cJSON_AddTodToObject(cJSON * const obj, char const * const key, poolstate_tod_t const * const tod);
void cJSON_AddVersionToObject(cJSON * const obj, char const * const key, poolstate_version_t const * const version);
void cJSON_AddSystemToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);

void cJSON_AddThermostatToObject(cJSON * const obj, char const * const key, poolstate_thermo_t const * const thermostat, bool const showTemp, bool showSp, bool const showSrc, bool const showHeating);
void cJSON_AddThermosToObject_generic(cJSON * const obj, char const * const key, poolstate_thermo_t const * thermos, bool const showTemp, bool showSp, bool const showSrc, bool const showHeating);
void cJSON_AddThermosToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);

void cJSON_AddScheduleToObject(cJSON * const obj, char const * const key, poolstate_sched_t const * const sched, bool const showSched);
void cJSON_AddSchedsToObject_generic(cJSON * const obj, char const * const key, poolstate_sched_t const * scheds, bool const showSched);
void cJSON_AddSchedsToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);

void cJSON_AddTempToObject(cJSON * const obj, char const * const key, poolstate_temp_t const * const temp);
void cJSON_AddTempsToObject(cJSON * const obj, char const * const key, poolstate_t const * state);
void cJSON_AddModesToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);
void cJSON_AddCircuitsToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);
void cJSON_AddStateToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);
void cJSON_AddPumpPrgToObject(cJSON * const obj, char const * const key, uint16_t const value);
void cJSON_AddPumpCtrlToObject(cJSON * const obj, char const * const key, uint8_t const ctrl);
void cJSON_AddPumpModeToObject(cJSON * const obj, char const * const key, uint8_t const mode);
void cJSON_AddPumpRunningToObject(cJSON * const obj, char const * const key, bool const running);
void cJSON_AddPumpStatusToObject(cJSON * const obj, char const * const key, poolstate_pump_t const * const pump);
void cJSON_AddPumpToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);
void cJSON_AddChlorRespToObject(cJSON * const obj, char const * const key, poolstate_chlor_t const * const chlor);
void cJSON_AddChlorToObject(cJSON * const obj, char const * const key, poolstate_t const * const state);
char const * poolstate_to_json(poolstate_t const * const state, poolstate_elem_typ_t const typ);

/* poolstate_get.c */
typedef char * poolstate_get_value_t;
typedef struct poolstate_get_params_t {
    poolstate_elem_typ_t elem_typ;
    uint8_t              elem_sub_typ;
    uint8_t const        idx;
} poolstate_get_params_t;
esp_err_t poolstate_get_value(poolstate_t const * const state, poolstate_get_params_t const * const params, poolstate_get_value_t * value);

/* poolstate_str.c */
const char * poolstate_chlor_status_str(poolstate_chlor_status_t const chlor_state_id);
const char * poolstate_thermo_str(poolstate_thermo_typ_t const thermostat_id);
const char * poolstate_temp_str(poolstate_temp_typ_t const temp_id);
int poolstate_temp_nr(char const * const temp_str);
int poolstate_thermo_nr(char const * const thermostat_str);
