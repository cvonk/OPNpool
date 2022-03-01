/**
* @brief Listen for commands over MQTT, and support MQTT device discovery.
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020-2022, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <strings.h>
#include <esp_system.h>
#include <esp_log.h>
#include <time.h>

#include "skb/skb.h"
#include "datalink/datalink.h"
#include "datalink/datalink_pkt.h"
#include "network/network.h"
#include "poolstate/poolstate.h"

#include "ipc/ipc.h"
#include "utils/utils.h"
#include "hass_task.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "hass_task";

typedef struct dispatch_hass_t {
    hass_dev_typ_t     dev_typ;
    char const * const id;
    char const * const name;
    char const * const unit;
} dispatch_hass_t;

typedef void      (* dispatch_init_fnc_t)(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg);
typedef esp_err_t (* dispatch_state_fnc_t)(poolstate_t const * const state, poolstate_get_params_t const * const params, dispatch_hass_t const * const hass, ipc_t const * const ipc);
typedef esp_err_t (* dispatch_set_fnc_t)(char const * const subtopic, poolstate_get_params_t const * const params, char const * const value_str, datalink_pkt_t * const pkt);

typedef struct dispatch_t {
    dispatch_hass_t          hass;
    struct {
        dispatch_init_fnc_t  init;
        dispatch_state_fnc_t state;
        dispatch_set_fnc_t   set;
    } fnc;
    poolstate_get_params_t   fnc_params;
} dispatch_t;

/*
 * Parses a topic string, splitting it at each '/' delimiter
 */

static uint
_parse_topic(char * data, char * args[], uint const args_len) {

    uint ii = 0;
    char const * const delim = "/";
    char * save;
    char * p = strtok_r(data, delim, &save);
    while (p && ii < args_len) {
        if (CONFIG_POOL_DBGLVL_HASSTASK > 1) {
            ESP_LOGI(TAG, "%s args[%u] = \"%s\"", __func__, ii, p);
        }
        args[ii++] = p;
        p = strtok_r(NULL, delim, &save);
    }
    return ii;
}

/*
 * Creates a MQTT discovery message for a `circuit`.
 * It `set_topic` is specified, it also create a MQTT topic(s) that the device will listen to
 * to let other MQTT clients change pool controller settings.
 */

static void
_circuit_init(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg)
{
    if (set_topics) {
        assert( asprintf(set_topics++, "%s/%s", base, "set") >= 0 );
    }
    assert( asprintf(cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                    "\"~\":\"%s\","
                    "\"name\":\"Pool %s\","
                    "\"cmd_t\":\"~/set\","
                    "\"stat_t\":\"~/state\"}",
                    base, base, hass->name) >= 0 );
}

/*
 * Publishes the state of a circuit to MQTT (by sending a message to the MQTT task).
 * Called as a registered function from hass_tx_state_to_mqtt().
 */

static esp_err_t
_circuit_state(poolstate_t const * const state, poolstate_get_params_t const * const params, dispatch_hass_t const * const hass, ipc_t const * const ipc)
{
    uint8_t const value = state->circuits.active[params->idx];
    char * combined;
    assert( asprintf(&combined, "homeassistant/%s/pool/%s/state"
                     "\t"
                     "%s",
                     hass_dev_typ_str(hass->dev_typ), hass->id, value ? "ON" : "OFF") >= 0 );
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
    free(combined);
    return ESP_OK;
}

/*
 * Creates a `network_msg_ctrl_circuit_set` message and returns ESP_OK.
 * Called as a registered function from hass_create_message().
 */

static esp_err_t
_circuit_set(char const * const subtopic, poolstate_get_params_t const * const params, char const * const value_str, datalink_pkt_t * const pkt)
{
    uint8_t const circuit_min_1 = params->idx;
    uint8_t const value = strcmp(value_str, "ON") == 0;

    network_msg_ctrl_circuit_set_t circuit_set = {
            .circuit = circuit_min_1 +1,
            .value = value,
    };
    network_msg_t msg = {
        .typ = MSG_TYP_CTRL_CIRCUIT_SET,
        .u.ctrl_circuit_set = &circuit_set,
    };
    if (network_create_msg(&msg, pkt)) {
        if (CONFIG_POOL_DBGLVL_HASSTASK > 1) {
            ESP_LOGW(TAG, "%s: circuit=%u, value=%u, pkt=%p", __func__, circuit_min_1, value, pkt);
        }
        return ESP_OK;
    }
    free(pkt);
    return ESP_FAIL;
}

/*
 * Creates a MQTT discovery message.
 * It `set_topic` is specified, it also create a MQTT topic(s) that the device will listen to
 * to let other MQTT clients change pool controller settings.
 */

static void
_generic_init(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg)
{
    assert( asprintf(cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                     "\"~\":\"%s\","
                     "\"name\":\"Pool %s\","
                     "\"stat_t\":\"~/state\","
                     "\"unit_of_meas\":\"%s\"}",
                     base, base, hass->name, hass->unit ? hass->unit : "") >= 0 );
}

/*
 * Publishes a generic state to MQTT (by sending a message to the MQTT task).
 * Called as a registered function from hass_tx_state_to_mqtt().
 */

static esp_err_t
_generic_state(poolstate_t const * const state, poolstate_get_params_t const * const params, dispatch_hass_t const * const hass, ipc_t const * const ipc)
{
    poolstate_get_value_t value;
    if (poolstate_get_value(state, params, &value) == ESP_OK) {
        char * combined;
        assert( asprintf(&combined, "homeassistant/%s/pool/%s/state"
                        "\t"
                        "%s",
                        hass_dev_typ_str(hass->dev_typ), hass->id, value) >= 0 );
        free(value);
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
        free(combined);
        return ESP_OK;
    }
    return ESP_FAIL;
}

/*
 * Creates a MQTT discovery message for a `thermostat`.
 * It `set_topic` is specified, it also create a MQTT topic(s) that the device will listen to
 * to let other MQTT clients change pool controller settings.
 */

static void
_thermo_init(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg)
{
    if (set_topics) {
        assert( asprintf(set_topics++, "%s/%s", base, "set_mode") >= 0 );
        assert( asprintf(set_topics++, "%s/%s", base, "set_temp") >= 0 );
    }
    assert( asprintf(cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                     "\"~\":\"%s\","
                     "\"name\":\"Pool %s\","
                     "\"mode_cmd_t\":\"~/set_mode\","
                     "\"temp_cmd_t\":\"~/set_temp\","
                     "\"mode_stat_tpl\":\"{{ value_json.mode }}\","
                     "\"mode_stat_t\":\"~/state\","
                     "\"temp_stat_tpl\":\"{{ value_json.target_temp }}\","
                     "\"temp_stat_t\":\"~/state\","
                     "\"curr_temp_t\":\"~/state\","
                     "\"curr_temp_tpl\":\"{{ value_json.current_temp }}\","
                     "\"min_temp\":15,"
                     "\"max_temp\":110,"
                     "\"temp_step\":1,"
                     "\"temp_unit\":\"%s\","
                     "\"avty_t\":\"~/available\","
                     "\"pl_avail\":\"online\","
                     "\"pl_not_avail\":\"offline\","
                     "\"modes\":[\"off\",\"heat\"]}",
                     base, base, hass->name, hass->unit ? hass->unit : "") >= 0);
}

/*
 * Publishes the state a thermostat to MQTT (by sending a message to the MQTT task).
 * Called as a registered function from hass_tx_state_to_mqtt().
 */

static esp_err_t
_thermo_state(poolstate_t const * const state, poolstate_get_params_t const * const params, dispatch_hass_t const * const hass, ipc_t const * const ipc)
{
    uint8_t const idx = params->idx;
    poolstate_thermo_t const * const thermostat = &state->thermos[idx];
    uint8_t const heat_src = thermostat->heat_src;
    uint8_t const target_temp = thermostat->set_point;
    uint8_t const current_temp = thermostat->temp;
    char * combined;
    assert( asprintf(&combined, "homeassistant/%s/pool/%s/state\t{"
                     "\"mode\":\"%s\","
                     "\"target_temp\":%u,"
                     "\"current_temp\":%u}",
                     hass_dev_typ_str(hass->dev_typ), hass->id, network_heat_src_str(heat_src), target_temp, current_temp) >= 0 );
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
    free(combined);
    assert( asprintf(&combined, "homeassistant/%s/pool/%s/available\t"
                     "online",
                     hass_dev_typ_str(hass->dev_typ), hass->id) >= 0 );
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
    free(combined);
    return ESP_OK;
}

/*
 * Creates a `network_msg_ctrl_heat_set` message and returns ESP_OK.
 * Called as a registered function from hass_create_message().
 */

static esp_err_t
_thermo_set(char const * const subtopic, poolstate_get_params_t const * const params, char const * const value_str, datalink_pkt_t * const pkt)
{
    poolstate_t state;
    poolstate_get(&state);

    uint8_t pool_set_point = state.thermos[POOLSTATE_THERMO_TYP_POOL].set_point;
    uint8_t pool_heat_src = state.thermos[POOLSTATE_THERMO_TYP_POOL].heat_src;
    uint8_t spa_set_point = state.thermos[POOLSTATE_THERMO_TYP_SPA].set_point;
    uint8_t spa_heat_src = state.thermos[POOLSTATE_THERMO_TYP_SPA].heat_src;

    poolstate_thermo_typ_t const thermostat_nr = POOLSTATE_THERMO_TYP_POOL;  // 2BD: should be based on the topic
    if (strcmp(subtopic, "set_mode") == 0) {
        int value;
        if ((value = network_heat_src_nr(value_str)) >= 0) {
            if (thermostat_nr == POOLSTATE_THERMO_TYP_POOL) {
                pool_heat_src = value;
            } else {
                spa_heat_src = value;
            }
        }
    } else if (strcmp(subtopic, "set_temp") == 0) {
        uint8_t const value = atoi(value_str);
        if (thermostat_nr == POOLSTATE_THERMO_TYP_POOL) {
            pool_set_point = value;
        } else {
            spa_set_point = value;
        }
    }
    network_msg_ctrl_heat_set_t heat_set = {
            .poolTempSetpoint = pool_set_point,
            .spaTempSetpoint = spa_set_point,
            .heatSrc = (pool_heat_src & 0x03) | (spa_heat_src << 2),
    };
    network_msg_t msg = {
            .typ = MSG_TYP_CTRL_HEAT_SET,
            .u.ctrl_heat_set = &heat_set,
    };
    if (network_create_msg(&msg, pkt)) {
        if (CONFIG_POOL_DBGLVL_HASSTASK > 1) {
            ESP_LOGW(TAG, "%s pkt=%p", __func__, pkt);
        }
        return ESP_OK;
    }
    free(pkt);
    return ESP_FAIL;
}

static dispatch_t _dispatches[] = {
    { { HASS_DEV_TYP_switch,  "pool_circuit", "Pool circuit", NULL  }, { _circuit_init, _circuit_state, _circuit_set }, { 0,                         0,                               NETWORK_CIRCUIT_POOL } },
    { { HASS_DEV_TYP_switch,  "spa_circuit",  "Spa circuit",  NULL  }, { _circuit_init, _circuit_state, _circuit_set }, { 0,                         0,                               NETWORK_CIRCUIT_SPA } },
    { { HASS_DEV_TYP_switch,  "aux1_circuit", "AUX1 circuit", NULL  }, { _circuit_init, _circuit_state, _circuit_set }, { 0,                         0,                               NETWORK_CIRCUIT_AUX1 } },
    { { HASS_DEV_TYP_switch,  "aux2_circuit", "AUX2 circuit", NULL  }, { _circuit_init, _circuit_state, _circuit_set }, { 0,                         0,                               NETWORK_CIRCUIT_AUX2 } },
    { { HASS_DEV_TYP_switch,  "aux3_circuit", "AUX3 circuit", NULL  }, { _circuit_init, _circuit_state, _circuit_set }, { 0,                         0,                               NETWORK_CIRCUIT_AUX3 } },
    { { HASS_DEV_TYP_switch,  "ft1_circuit",  "Ft1 circuit",  NULL  }, { _circuit_init, _circuit_state, _circuit_set }, { 0,                         0,                               NETWORK_CIRCUIT_FT1 } },
    { { HASS_DEV_TYP_switch,  "ft2_circuit",  "Ft2 circuit",  NULL  }, { _circuit_init, _circuit_state, _circuit_set }, { 0,                         0,                               NETWORK_CIRCUIT_FT2 } },
    { { HASS_DEV_TYP_switch,  "ft3_circuit",  "Ft3 circuit",  NULL  }, { _circuit_init, _circuit_state, _circuit_set }, { 0,                         0,                               NETWORK_CIRCUIT_FT3 } },
    { { HASS_DEV_TYP_switch,  "ft4_circuit",  "Ft4 circuit",  NULL  }, { _circuit_init, _circuit_state, _circuit_set }, { 0,                         0,                               NETWORK_CIRCUIT_FT4 } },
    { { HASS_DEV_TYP_climate, "pool_heater",  "pool heater",  "°F"  }, { _thermo_init,  _thermo_state,  _thermo_set  }, { 0,                         0,                               POOLSTATE_THERMO_TYP_POOL } },
    { { HASS_DEV_TYP_climate, "spa_heater",   "spa heater",   "°F"  }, { _thermo_init,  _thermo_state,  _thermo_set  }, { 0,                         0,                               POOLSTATE_THERMO_TYP_SPA } },
    { { HASS_DEV_TYP_sensor,  "sch1_circuit", "sch1 circuit", NULL  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_SCHED,  POOLSTATE_ELEM_SCHED_TYP_CIRCUIT, 0 } },
    { { HASS_DEV_TYP_sensor,  "sch1_start",   "sch1 start",   NULL  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_SCHED,  POOLSTATE_ELEM_SCHED_TYP_START, 0 } },
    { { HASS_DEV_TYP_sensor,  "sch1_stop",    "sch1 stop",    NULL  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_SCHED,  POOLSTATE_ELEM_SCHED_TYP_STOP, 0 } },
    { { HASS_DEV_TYP_sensor,  "sch2_circuit", "sch2 circuit", NULL  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_SCHED,  POOLSTATE_ELEM_SCHED_TYP_CIRCUIT, 1 } },
    { { HASS_DEV_TYP_sensor,  "sch2_start",   "sch2 start",   NULL  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_SCHED,  POOLSTATE_ELEM_SCHED_TYP_START,  1 } },
    { { HASS_DEV_TYP_sensor,  "sch2_stop",    "sch2 stop",    NULL  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_SCHED,  POOLSTATE_ELEM_SCHED_TYP_STOP,   1 } },
    { { HASS_DEV_TYP_sensor,  "air_temp",     "air temp",     "°F"  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_TEMP,   POOLSTATE_ELEM_TEMP_TYP_TEMP,    POOLSTATE_TEMP_TYP_AIR  } },
    { { HASS_DEV_TYP_sensor,  "water_temp",   "water temp",   "°F"  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_THERMO, POOLSTATE_ELEM_THERMO_TYP_TEMP,  POOLSTATE_THERMO_TYP_POOL } },
    { { HASS_DEV_TYP_sensor,  "system_time",  "time",         NULL  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_SYSTEM, POOLSTATE_ELEM_SYSTEM_TYP_TIME,  0 } },
    { { HASS_DEV_TYP_sensor,  "pump_pwr",     "pump pwr",     "W"   }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_PUMP,   POOLSTATE_ELEM_PUMP_TYP_PWR,     0 } },
    { { HASS_DEV_TYP_sensor,  "pump_speed",   "pump speed",   "rpm" }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_PUMP,   POOLSTATE_ELEM_PUMP_TYP_RPM,     0 } },
    { { HASS_DEV_TYP_sensor,  "pump_status",  "pump status",  NULL  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_PUMP,   POOLSTATE_ELEM_PUMP_TYP_STATUS,  0 } },
    { { HASS_DEV_TYP_sensor,  "chlor_pct",    "chlor pct",    "%"   }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_CHLOR,  POOLSTATE_ELEM_CHLOR_TYP_PCT,    0 } },
    { { HASS_DEV_TYP_sensor,  "chlor_salt",   "chlor salt",   "ppm" }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_CHLOR,  POOLSTATE_ELEM_CHLOR_TYP_SALT,   0 } },
    { { HASS_DEV_TYP_sensor,  "chlor_status", "chlor status", NULL  }, { _generic_init,  _generic_state,  NULL       }, { POOLSTATE_ELEM_TYP_CHLOR,  POOLSTATE_ELEM_CHLOR_TYP_STATUS, 0 } },
};

/*
 * Publishes the state information to MQTT
 */

esp_err_t
hass_tx_state_to_mqtt(poolstate_t const * const state, ipc_t const * const ipc)
{
    dispatch_t const * dispatch = _dispatches;
    for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
        if (dispatch->fnc.state) {

            // the registered `state` function publishes the state using the MQTT task.

            dispatch->fnc.state(state, &dispatch->fnc_params, &dispatch->hass, ipc);
        }
    }
#if 0
    char const * const json = poolstate_to_json(state, POOLSTATE_ELEM_TYP_ALL);
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
    free((void *)json);
#endif
    return ESP_OK;
}

/*
 * Create a message to be sent to the pool controller
 */

esp_err_t
hass_create_message(char * const topic, char const * const value_str, datalink_pkt_t * const pkt)
{
    if (CONFIG_POOL_DBGLVL_HASSTASK > 1) {
        ESP_LOGI(TAG, "topic = \"%s\", value = \"%s\"", topic, value_str);
    }
    char * args[5];
    uint8_t argc = _parse_topic(topic, args, ARRAY_SIZE(args));

    if (argc >= 5) {
        char const * const dev_typ = args[1];
        char const * const hass_id = args[3];
        char const * const subtopic = args[4];

        dispatch_t const * dispatch = _dispatches;
        for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
            if (strcmp(dev_typ, hass_dev_typ_str(dispatch->hass.dev_typ)) == 0 &&
                strcmp(hass_id, dispatch->hass.id) == 0 && dispatch->fnc.set) {

                // The registered `set` function will create a corresponding message if
                // successfull return ESP_OK.  In that case, the caller will call
                // datalink_tx_pkt_queue() to transmit the message.

                return dispatch->fnc.set(subtopic, &dispatch->fnc_params, value_str, pkt);
            }
        }
    }
    return ESP_FAIL;
}

/*
 * The hass_task's duties are
 *   - listen for commands over MQTT,
 *   - support MQTT device discovery.
 */

void
hass_task(void * ipc_void)
{
    bool mqtt_subscribe = true;
 	ipc_t * const ipc = ipc_void;

    while (1) {
        dispatch_t const * dispatch = _dispatches;
        for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {

            if (dispatch->fnc.init) {
                char * base;
                assert( asprintf(&base, "homeassistant/%s/pool/%s", hass_dev_typ_str(dispatch->hass.dev_typ), dispatch->hass.id) >= 0 );

                char * cfg;
                if (mqtt_subscribe) {
                    char * set_topics[4] = {};

                    // the registered `init` function will create a MQTT topic(s) that the device will
                    // listen to to let other MQTT clients change pool controller settings such as
                    // thermostat set point or heating mode.  It also creates a MQTT discovery message.

                    dispatch->fnc.init(base, &dispatch->hass, set_topics, &cfg);

                    // subscribe to the MQTT topic(s) (by sending a request to the MQTT task)

                    char * * topic = set_topics;
                    for (uint jj = 0; jj < ARRAY_SIZE(set_topics) && *topic; jj++, topic++) {
                        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_SUBSCRIBE, *topic, ipc);
                        free(*topic);
                    }
                } else {

                    // the registered `init` function will create a MQTT discovery message.

                    dispatch->fnc.init(base, &dispatch->hass, NULL, &cfg);
                }

                // publish the MQTT discovery message (by sendint it to the MQTT task)

                ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, cfg, ipc);
                free(cfg);
                free(base);
            }
        }
        mqtt_subscribe = false;
        vTaskDelay((TickType_t)5 * 60 * 1000 / portTICK_PERIOD_MS);
    }
}
