/**
* @brief hass, interacts with home assistant (home-assistant.io
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
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

static void
_circuit_init(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg)
{
    assert( asprintf(set_topics++, "%s/%s", base, "set") >= 0 );
    assert( asprintf(cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                    "\"~\":\"%s\","
                    "\"name\":\"Pool %s\","
                    "\"cmd_t\":\"~/set\","
                    "\"stat_t\":\"~/state\"}",
                    base, base, hass->name) >= 0 );
}

static esp_err_t
_circuit_state(poolstate_t const * const state, poolstate_get_params_t const * const params, dispatch_hass_t const * const hass, ipc_t const * const ipc)
{
    uint8_t const value = state->circuits.active[params->idx];
    char * combined;
    assert( asprintf(&combined, "homeassistant/%s/%s/%s/state"
                     "\t"
                     "%s", hass_dev_typ_str(hass->dev_typ), ipc->dev.name, hass->id, value ? "ON" : "OFF") >= 0 );
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
    free(combined);
    return ESP_OK;
}

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
    if (network_tx_msg(&msg, pkt)) {
        if (CONFIG_POOL_DBGLVL_HASSTASK > 1) {
            ESP_LOGW(TAG, "%s circuit=%u, value=%u, pkt=%p", __func__, circuit_min_1, value, pkt);
        }
        return ESP_OK;
    }
    free(pkt);
    return ESP_FAIL;
}

static void
_json_init(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg)
{
    assert( asprintf(cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                     "\"~\":\"%s\","
                     "\"name\":\"Pool %s\","
                     "\"stat_t\":\"~/state\","
                     "\"unit_of_meas\":\"%s\"}",
                     base, base, hass->name, hass->unit ? hass->unit : "") >= 0 );
}

static esp_err_t
_json_state(poolstate_t const * const state, poolstate_get_params_t const * const params, dispatch_hass_t const * const hass, ipc_t const * const ipc)
{
    poolstate_get_value_t value;
    if (poolstate_get_value(state, params, &value) == ESP_OK) {
        char * combined;
        assert( asprintf(&combined, "homeassistant/%s/%s/%s/state"
                        "\t"
                        "%s",
                        hass_dev_typ_str(hass->dev_typ), ipc->dev.name, hass->id,
                        value) >= 0 );
        free(value);
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
        free(combined);
        return ESP_OK;
    }
    return ESP_FAIL;
}

static void
_thermostat_init(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg)
{
    assert( asprintf(set_topics++, "%s/%s", base, "set_mode") >= 0 );
    assert( asprintf(set_topics++, "%s/%s", base, "set_temp") >= 0 );
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
                     "\"avty_t\":\"~/available\","
                     "\"pl_avail\":\"online\","
                     "\"pl_not_avail\":\"offline\","
                     "\"modes\":[\"off\",\"heat\"]}", base, base, hass->name) >= 0);
}

static esp_err_t
_thermostat_state(poolstate_t const * const state, poolstate_get_params_t const * const params, dispatch_hass_t const * const hass, ipc_t const * const ipc)
{
    uint8_t const idx = params->idx;
    poolstate_thermostat_t const * const thermostat = &state->thermostats[idx];
    uint8_t const heat_src = thermostat->heat_src;
    uint8_t const target_temp = thermostat->set_point;
    uint8_t const current_temp = thermostat->temp;
    char * combined;
    assert( asprintf(&combined, "homeassistant/%s/%s/%s/state\t{"
                     "\"mode\":\"%s\","
                     "\"target_temp\":%u,"
                     "\"current_temp\":%u}", hass_dev_typ_str(hass->dev_typ), ipc->dev.name, hass->id,
                     network_heat_src_str(heat_src), target_temp, current_temp) >= 0 );
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
    free(combined);
    assert( asprintf(&combined, "homeassistant/%s/%s/%s/available\t"
                     "online",
                     hass_dev_typ_str(hass->dev_typ), ipc->dev.name, hass->id) >= 0 );
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
    free(combined);
    return ESP_OK;
}

static esp_err_t
_thermostat_set(char const * const subtopic, poolstate_get_params_t const * const params, char const * const value_str, datalink_pkt_t * const pkt)
{
    poolstate_t state;
    poolstate_get(&state);

    uint8_t pool_set_point = state.thermostats[POOLSTATE_THERMOSTAT_POOL].set_point;
    uint8_t pool_heat_src = state.thermostats[POOLSTATE_THERMOSTAT_POOL].heat_src;
    uint8_t spa_set_point = state.thermostats[POOLSTATE_THERMOSTAT_SPA].set_point;
    uint8_t spa_heat_src = state.thermostats[POOLSTATE_THERMOSTAT_SPA].heat_src;

    poolstate_thermostats_t const thermostat_nr = POOLSTATE_THERMOSTAT_POOL;  // 2BD: should be based on the topic
    if (strcmp(subtopic, "set_mode") == 0) {
        int value;
        if ((value = network_heat_src_nr(value_str)) >= 0) {
            if (thermostat_nr == POOLSTATE_THERMOSTAT_POOL) {
                pool_heat_src = value;
            } else {
                spa_heat_src = value;
            }
        }
    } else if (strcmp(subtopic, "set_temp") == 0) {
        uint8_t const value = atoi(value_str);
        if (thermostat_nr == POOLSTATE_THERMOSTAT_POOL) {
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
    if (network_tx_msg(&msg, pkt)) {
        if (CONFIG_POOL_DBGLVL_HASSTASK > 1) {
            ESP_LOGW(TAG, "%s pkt=%p", __func__, pkt);
        }
        return ESP_OK;
    }
    free(pkt);
    return ESP_FAIL;
}

static dispatch_t _dispatches[] = {
    { { HASS_DEV_TYP_switch,  "pool_circuit", "Pool circuit", NULL  }, { _circuit_init,    _circuit_state,    _circuit_set    }, { 0,                             0,                                   NETWORK_CIRCUIT_POOL } },
    { { HASS_DEV_TYP_switch,  "spa_circuit",  "Spa circuit",  NULL  }, { _circuit_init,    _circuit_state,    _circuit_set    }, { 0,                             0,                                   NETWORK_CIRCUIT_SPA } },
    { { HASS_DEV_TYP_switch,  "aux1_circuit", "AUX1 circuit", NULL  }, { _circuit_init,    _circuit_state,    _circuit_set    }, { 0,                             0,                                   NETWORK_CIRCUIT_AUX1 } },
    { { HASS_DEV_TYP_climate, "pool_heater",  "pool heater",  NULL  }, { _thermostat_init, _thermostat_state, _thermostat_set }, { 0,                             0,                                   POOLSTATE_THERMOSTAT_POOL } },
    { { HASS_DEV_TYP_sensor,  "pool_start",   "pool start",   NULL  }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_THERMOSTAT, POOLSTATE_ELEM_THERMOSTAT_TYP_START, POOLSTATE_THERMOSTAT_POOL } },
    { { HASS_DEV_TYP_sensor,  "pool_stop",    "pool stop",    NULL  }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_THERMOSTAT, POOLSTATE_ELEM_THERMOSTAT_TYP_START, POOLSTATE_THERMOSTAT_POOL } },
    { { HASS_DEV_TYP_climate, "spa_heater",   "spa heater",   NULL  }, { _thermostat_init, _thermostat_state, _thermostat_set }, { 0,                             0,                                   POOLSTATE_THERMOSTAT_SPA } },
    { { HASS_DEV_TYP_sensor,  "spa_start",    "spa start",    NULL  }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_THERMOSTAT, POOLSTATE_ELEM_THERMOSTAT_TYP_START, POOLSTATE_THERMOSTAT_SPA } },
    { { HASS_DEV_TYP_sensor,  "spa_stop",     "spa stop",     NULL  }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_THERMOSTAT, POOLSTATE_ELEM_THERMOSTAT_TYP_START, POOLSTATE_THERMOSTAT_SPA } },
    { { HASS_DEV_TYP_sensor,  "air_temp",     "air temp",     "°F"  }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_TEMP,       POOLSTATE_ELEM_TEMP_TYP_TEMP,        POOLSTATE_TEMP_AIR  } },
    { { HASS_DEV_TYP_sensor,  "water_temp",   "water temp",   "°F"  }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_THERMOSTAT, POOLSTATE_ELEM_THERMOSTAT_TYP_TEMP,  POOLSTATE_THERMOSTAT_POOL } },
    { { HASS_DEV_TYP_sensor,  "system_time",  "time",         NULL  }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_SYSTEM,     POOLSTATE_ELEM_SYSTEM_TYP_TIME,      0 } },
    { { HASS_DEV_TYP_sensor,  "pump_pwr",     "pump pwr",     "W"   }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_PUMP,       POOLSTATE_ELEM_PUMP_TYP_PWR,         0 } },
    { { HASS_DEV_TYP_sensor,  "pump_speed",   "pump speed",   "rpm" }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_PUMP,       POOLSTATE_ELEM_PUMP_TYP_RPM,         0 } },
    { { HASS_DEV_TYP_sensor,  "pumps_tatus",  "pump status",  NULL  }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_PUMP,       POOLSTATE_ELEM_PUMP_TYP_STATUS,      0 } },
    { { HASS_DEV_TYP_sensor,  "chlor_pct",    "chlor pct",    "%"   }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_CHLOR,      POOLSTATE_ELEM_CHLOR_TYP_PCT,        0 } },
    { { HASS_DEV_TYP_sensor,  "chlor_salt",   "chlor salt",   "ppm" }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_CHLOR,      POOLSTATE_ELEM_CHLOR_TYP_SALT,       0 } },
    { { HASS_DEV_TYP_sensor,  "chlor_status", "chlor status", NULL  }, { _json_init,       _json_state,       NULL            }, { POOLSTATE_ELEM_TYP_CHLOR,      POOLSTATE_ELEM_CHLOR_TYP_STATUS,     0 } },
};

esp_err_t
hass_tx_state(poolstate_t const * const state, ipc_t const * const ipc)
{
    dispatch_t const * dispatch = _dispatches;
    for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
        if (dispatch->fnc.state) {

            dispatch->fnc.state(state, &dispatch->fnc_params, &dispatch->hass, ipc);
        }
    }
    char const * const json = poolstate_to_json(state, POOLSTATE_ELEM_TYP_ALL);
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
    free((void *)json);
    return ESP_OK;
}

esp_err_t
hass_rx_set(char * const topic, char const * const value_str, datalink_pkt_t * const pkt)
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
                strcmp(hass_id, dispatch->hass.name) && dispatch->fnc.set) {

                return dispatch->fnc.set(subtopic, &dispatch->fnc_params, value_str, pkt);
            }
        }
    }
    return ESP_FAIL;
}

/**
 * every 5 minutes publish .../config topics to homeassistant over MQTT
 **/
void
hass_task(void * ipc_void)
{
    bool first_time = true;
 	ipc_t * const ipc = ipc_void;

    while (1) {
        dispatch_t const * dispatch = _dispatches;
        for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
            if (dispatch->fnc.init) {
                char * set_topics[4];
                for (uint jj = 0; jj < ARRAY_SIZE(set_topics); jj++) {
                    set_topics[jj] = NULL;
                }
                char * base;
                assert( asprintf(&base, "homeassistant/%s/pool/%s", hass_dev_typ_str(dispatch->hass.dev_typ), dispatch->hass.id) >= 0 );

                char * cfg;
                dispatch->fnc.init(base, &dispatch->hass, set_topics, &cfg);

                if (first_time) {
                    for (uint jj = 0; jj < ARRAY_SIZE(set_topics) && set_topics[jj]; jj++) {
                        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_SUBSCRIBE, set_topics[jj], ipc);
                        free(set_topics[jj]);
                    }
                    first_time = false;
                }
                ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, cfg, ipc);
                free(cfg);
                free(base);
            }
        }
        vTaskDelay((TickType_t)5 * 60 * 1000 / portTICK_PERIOD_MS);
    }
}
