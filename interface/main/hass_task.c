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

static char const * const TAG = "hass";

typedef struct dispatch_hass_t {
    hass_dev_typ_t     dev_typ;
    char const * const name;
    char const * const id;
} dispatch_hass_t;

typedef void (* dispatch_init_fnc_t)(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg);
typedef esp_err_t (* dispatch_state_fnc_t)(poolstate_t const * const state, uint8_t const idx, dispatch_hass_t const * const hass, ipc_t const * const ipc);
typedef esp_err_t (* dispatch_set_fnc_t)(char const * const subtopic, uint8_t const idx, char const * const value_str, datalink_pkt_t * const pkt);

typedef struct dispatch_t {
    dispatch_hass_t      hass;
    uint8_t              dev_idx;
    dispatch_init_fnc_t  init_fnc;
    dispatch_state_fnc_t state_fnc;
    dispatch_set_fnc_t   set_fnc;
} dispatch_t;

static uint
_parse_topic(char * data, char * args[], uint const args_len) {

    uint ii = 0;
    char const * const delim = "/";
    char * save;
    char * p = strtok_r(data, delim, &save);
    while (p && ii < args_len) {
        args[ii++] = p;
        p = strtok_r(NULL, delim, &save);
    }
    return ii;
}

static void
_circuit_init(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg)
{
    assert( asprintf(set_topics++, "%s/%s", base, "set") >= 0);
    assert( asprintf(cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                    "\"~\":\"%s\","
                    "\"name\":\"pool %s\","
                    "\"cmd_t\":\"~/set\","
                    "\"stat_t\":\"~/state\"}",
                    base, base, hass->name) >= 0);
}

static esp_err_t
_circuit_state(poolstate_t const * const state, uint8_t const idx, dispatch_hass_t const * const hass, ipc_t const * const ipc)
{
    uint8_t const value = state->circuits.active[idx];
    char * combined;
    assert( asprintf(&combined, "homeassistant/%s/%s/%s/state"
                     "\t"
                     "%s", hass_dev_typ_str(hass->dev_typ), ipc->dev.name, hass->id, value ? "ON" : "OFF") >= 0);
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
    free(combined);
    return ESP_OK;
}

static esp_err_t
_circuit_set(char const * const subtopic, uint8_t const circuit_min_1, char const * const value_str, datalink_pkt_t * const pkt)
{
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
        ESP_LOGW(TAG, "%s pkt=%p", __func__, pkt);
        return ESP_OK;
    }
    free(pkt);
    return ESP_FAIL;
}

static void
_temp_init(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg)
{
    assert( asprintf(cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                    "\"~\":\"%s\","
                    "\"name\":\"pool %s\","
                    "\"stat_t\":\"~/state\"}",
                    base, base, hass->name) >= 0);
}

static esp_err_t
_temp_state(poolstate_t const * const state, uint8_t const idx, dispatch_hass_t const * const hass, ipc_t const * const ipc)
{
    int temp_nr;
    if ((temp_nr = poolstate_temp_nr(hass->id)) >= 0) {
        uint8_t const value = state->temps[idx].temp;
        char * combined;
        assert( asprintf(&combined, "homeassistant/%s/%s/%s/state"
                            "\t"
                            "%u", hass_dev_typ_str(hass->dev_typ), ipc->dev.name, hass->id, value) >= 0);
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
        free(combined);
    }
    return ESP_OK;
}

static void
_thermostat_init(char const * const base, dispatch_hass_t const * const hass, char * * set_topics, char * * const cfg)
{
    assert( asprintf(set_topics++, "%s/%s", base, "set_mode") >= 0);
    assert( asprintf(set_topics++, "%s/%s", base, "set_temp") >= 0);
    assert( asprintf(cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                    "\"~\":\"%s\","
                    "\"name\":\"pool %s\","
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
                    //"\"unique_id\":\"%s\","
                    "\"modes\":[\"off\",\"heat\"]"
                    "}",
                    base, base, hass->name) >= 0);
}

static esp_err_t
_thermostat_state(poolstate_t const * const state, uint8_t const idx, dispatch_hass_t const * const hass, ipc_t const * const ipc)
{
    int thermostat_nr;
    if ((thermostat_nr = poolstate_thermostat_nr(hass->id)) >= 0) {
        uint8_t const heat_src = state->thermostats[idx].heat_src;
        uint8_t const target_temp = state->thermostats[idx].set_point;
        uint8_t const current_temp = state->thermostats[idx].temp;
        char * combined;
        assert( asprintf(&combined, "homeassistant/%s/%s/%s/state"
                            "\t"
                            "{"
                            "\"mode\":\"%s\","
                            "\"target_temp\":%u,"
                            "\"current_temp\":%u}",
                            hass_dev_typ_str(hass->dev_typ), ipc->dev.name, hass->id, network_heat_src_str(heat_src), target_temp, current_temp) >= 0 );
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
        free(combined);
        assert( asprintf(&combined, "homeassistant/%s/%s/%s/available"
                            "\t"
                            "online",
                            hass_dev_typ_str(hass->dev_typ), ipc->dev.name, hass->id) >= 0 );
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
        free(combined);
    } else {
        ESP_LOGE(TAG, "thermostat not found (%s)", hass->id);
    }
    return ESP_OK;
}

static esp_err_t
_thermostat_set(char const * const subtopic, uint8_t const thermostat_nr, char const * const value_str, datalink_pkt_t * const pkt)
{
    poolstate_t state;
    poolstate_get(&state);
    uint8_t pool_set_point = state.thermostats[POOLSTATE_THERMOSTAT_POOL].set_point;
    uint8_t spa_set_point = state.thermostats[POOLSTATE_THERMOSTAT_SPA].set_point;
    uint8_t pool_heat_src = state.thermostats[POOLSTATE_THERMOSTAT_POOL].heat_src;
    uint8_t spa_heat_src = state.thermostats[POOLSTATE_THERMOSTAT_SPA].heat_src;

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
        ESP_LOGW(TAG, "%s pkt=%p", __func__, pkt);
        return ESP_OK;
    }
    free(pkt);
    return ESP_FAIL;
}

static dispatch_t _dispatches[] = {
    { { HASS_DEV_TYP_switch,  "aux1 circuit", "aux1_circuit" }, NETWORK_CIRCUIT_AUX1,      _circuit_init,    _circuit_state,    _circuit_set,  },
    { { HASS_DEV_TYP_switch,  "pool circuit", "pool_circuit" }, NETWORK_CIRCUIT_POOL,      _circuit_init,    _circuit_state,    _circuit_set },
    { { HASS_DEV_TYP_sensor,  "air temp",     "air" },          POOLSTATE_TEMP_AIR,        _temp_init,       _temp_state,       NULL },
    { { HASS_DEV_TYP_climate, "heater",       "pool" },         POOLSTATE_THERMOSTAT_POOL, _thermostat_init, _thermostat_state, _thermostat_set },
};

esp_err_t
hass_tx_state(poolstate_t const * const state, ipc_t const * const ipc)
{
    dispatch_t const * dispatch = _dispatches;
    for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
        if (dispatch->state_fnc) {

            dispatch->state_fnc(state, dispatch->dev_idx, &dispatch->hass, ipc);
            break;
        }
    }  // for

    size_t json_size = 1024;
    char json[json_size];
    poolstate_to_json(state, json, json_size);
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
    return ESP_OK;
}

esp_err_t
hass_rx_mqtt(char * const topic, char const * const value_str, datalink_pkt_t * const pkt)
{
    char * args[5];
    uint8_t argc = _parse_topic(topic, args, ARRAY_SIZE(args));

    if (argc >= 5) {
        char const * const dev_typ = args[1];
        char const * const hass_id = args[3];
        char const * const subtopic = args[4];

        dispatch_t const * dispatch = _dispatches;
        for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
            if (strcmp(dev_typ, hass_dev_typ_str(dispatch->hass.dev_typ)) == 0 &&
                strcmp(hass_id, dispatch->hass.id) && dispatch->set_fnc) {

                return dispatch->set_fnc(subtopic, dispatch->dev_idx, value_str, pkt);
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
            if (dispatch->init_fnc) {
                char * set_topics[4];
                for (uint jj = 0; jj < ARRAY_SIZE(set_topics); jj++) {
                    set_topics[jj] = NULL;
                }
                char * base;
                assert( asprintf(&base, "homeassistant/%s/%s/%s", hass_dev_typ_str(dispatch->hass.dev_typ), ipc->dev.name, dispatch->hass.id) >= 0);

                char * cfg;
                dispatch->init_fnc(base, &dispatch->hass, set_topics, &cfg);

                if (first_time) {
                    for (uint jj = 0; set_topics[jj] && jj < ARRAY_SIZE(set_topics); jj++) {
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
