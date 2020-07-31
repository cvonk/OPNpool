/**
* @brief packet_task, packetizes RS485 byte stream from Pentair bus
 *
 * The Pentair controller uses two different protocols to communicate with its peripherals:
 *   - 	A5 has messages such as 0x00 0xFF <ldb> <sub> <dst> <src> <cfi> <len> [<data>] <chH> <ckL>
 *   -  IC has messages such as 0x10 0x02 <data0> <data1> <data2> .. <dataN> <ch> 0x10 0x03
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

#include "../skb/skb.h"
#include "../rs485/rs485.h"
#include "../datalink/datalink.h"
#include "../datalink/datalink_pkt.h"
#include "../network/network.h"
#include "../poolstate/poolstate.h"

#include "../ipc/ipc.h"
#include "../utils/utils.h"
#include "hass.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

static char const * const TAG = "hass";

typedef esp_err_t (* dispatch_fnc_t)(char const * const subtopic, uint8_t const idx, char const * const value_str, datalink_pkt_t * const pkt);

typedef struct dispatch_t {
    hass_dev_typ_t     dev_type;
    char const * const hass_name;
    char const * const dev_name;
    uint8_t            dev_idx;
    dispatch_fnc_t     set_fnc;
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

static esp_err_t
_dispatch_circuit_set(char const * const subtopic, uint8_t const circuit_min_1, char const * const value_str, datalink_pkt_t * const pkt)
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

static esp_err_t
_dispatch_thermostat_set(char const * const subtopic, uint8_t const thermostat_nr, char const * const value_str, datalink_pkt_t * const pkt)
{
    ESP_LOGW(TAG, "sub_topic = %s", subtopic);
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
    { HASS_DEV_TYP_switch,  "aux1 circuit", "aux1",  NETWORK_CIRCUIT_AUX1,      _dispatch_circuit_set },
    { HASS_DEV_TYP_switch,  "pool circuit", "pool",  NETWORK_CIRCUIT_POOL,      _dispatch_circuit_set },
    { HASS_DEV_TYP_sensor,  "air temp",     "air",   POOLSTATE_TEMP_AIR,        NULL },
    { HASS_DEV_TYP_climate, "heater",       "pool",  POOLSTATE_THERMOSTAT_POOL, _dispatch_thermostat_set },
};

esp_err_t
hass_rx_mqtt(char * const topic, char const * const value_str, datalink_pkt_t * const pkt)
{
    char * args[5];
    uint8_t argc = _parse_topic(topic, args, ARRAY_SIZE(args));

    if (argc >= 5) {
        char const * const dev_type = args[1];
        char const * const dev_name = args[3];
        char const * const sub_topic = args[4];

        dispatch_t const * dispatch = _dispatches;
        for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
            if (strcmp(dev_type, hass_dev_typ_str(dispatch->dev_type)) == 0 &&
                strcmp(dev_name, dispatch->dev_name) && dispatch->set_fnc) {

                return dispatch->set_fnc(sub_topic, dispatch->dev_idx, value_str, pkt);
            }
        }
    }
    return ESP_FAIL;
}

void
hass_init(ipc_t const * const ipc)
{
    dispatch_t const * dispatch = _dispatches;
    for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
        char * set_topics[4];
        for (uint jj = 0; jj < ARRAY_SIZE(set_topics); jj++) {
            set_topics[jj] = NULL;
        }
        char * base;
        assert( asprintf(&base, "homeassistant/%s/%s/%s", hass_dev_typ_str(dispatch->dev_type), ipc->dev.name, dispatch->dev_name) >= 0);
        char * cfg;
        switch (dispatch->dev_type) {
            case HASS_DEV_TYP_switch:
                assert( asprintf(&set_topics[0], "%s/%s", base, "set") >= 0);
                assert( asprintf(&cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                                "\"~\":\"%s\","
                                "\"name\":\"pool %s\","
                                "\"cmd_t\":\"~/set\","
                                "\"stat_t\":\"~/state\"}",
                                base, base, dispatch->hass_name) >= 0);
                break;
           case HASS_DEV_TYP_sensor:
                assert( asprintf(&cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                                "\"~\":\"%s\","
                                "\"name\":\"pool %s\","
                                "\"temp_unit\": \"°F\","
                                "\"stat_t\":\"~/state\"}",
                                base, base, dispatch->hass_name) >= 0);
                break;
           case HASS_DEV_TYP_climate:
                assert( asprintf(&set_topics[0], "%s/%s", base, "set_mode") >= 0);
                assert( asprintf(&set_topics[1], "%s/%s", base, "set_temp") >= 0);
                assert( asprintf(&cfg, "%s/config" "\t{"  // '\t' separates the topic and the message
                                "\"~\":\"%s\","
                                "\"name\":\"pool %s\","
                                "\"mode_cmd_t\":\"~/set_mode\","
                                "\"temp_cmd_t\":\"~/set_temp\","
                                "\"mode_stat_tpl\":\"\","
                                "\"mode_stat_t\":\"~/state\","
                                "\"temp_stat_tpl\":\"\","
                                "\"temp_stat_t\":\"~/state\","
                                "\"curr_temp_t\":\"~/state\","
                                "\"curr_temp_tpl\":\"\","
                                "\"min_temp\":15,"
                                "\"max_temp\":110,"
                                "\"temp_step\":1,"
                                "\"temp_unit\": \"°F\","
                                "\"modes\":[\"none\",\"heater\",\"solarpref\",\"solar\"]"
                                "}",
                                base, base, dispatch->hass_name) >= 0);
                break;
        }
        for (uint jj = 0; set_topics[jj] && jj < ARRAY_SIZE(set_topics); jj++) {
            ipc_send_to_mqtt(IPC_TO_MQTT_TYP_SUBSCRIBE, set_topics[jj], ipc);
            free(set_topics[jj]);
        }
        ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, cfg, ipc);
        free(cfg);
        free(base);
    }
}

#if 0
{
  "mode_cmd_t":"homeassistant/climate/livingroom/thermostatModeCmd",
  "mode_stat_t":"homeassistant/climate/livingroom/state",
  "mode_stat_tpl":"",
  "avty_t":"homeassistant/climate/livingroom/available",
  "pl_avail":"online",
  "pl_not_avail":"offline",
  "temp_cmd_t":"homeassistant/climate/livingroom/targetTempCmd",
  "temp_stat_t":"homeassistant/climate/livingroom/state",
  "temp_stat_tpl":"",
  "curr_temp_t":"homeassistant/climate/livingroom/state",
  "curr_temp_tpl":"",
  "min_temp":"15",
  "max_temp":"25",
  "temp_step":"0.5",
  "modes":["off", "heat"]
}
#endif

esp_err_t
hass_tx_state(poolstate_t const * const state, ipc_t const * const ipc)
{
    dispatch_t const * dispatch = _dispatches;
    for (uint ii = 0; ii < ARRAY_SIZE(_dispatches); ii++, dispatch++) {
        switch (dispatch->dev_type) {
            case HASS_DEV_TYP_switch: {
                uint8_t const value = state->circuits.active[dispatch->dev_idx];
                char * combined;
                assert( asprintf(&combined, "homeassistant/%s/%s/%s/state"
                                 "\t"
                                 "\"%s\"", hass_dev_typ_str(dispatch->dev_type), ipc->dev.name, dispatch->dev_name, value ? "ON" : "OFF") >= 0);
                ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
                free(combined);
                break;
            }
            case HASS_DEV_TYP_sensor: {
                int temp_nr;
                if ((temp_nr = poolstate_temp_nr(dispatch->dev_name)) >= 0) {
                    uint8_t const value = state->temps[dispatch->dev_idx].temp;
                    char * combined;
                    assert( asprintf(&combined, "homeassistant/%s/%s/%s/state"
                                     "\t"
                                     "\"%u\"", hass_dev_typ_str(dispatch->dev_type), ipc->dev.name, dispatch->dev_name, value) >= 0);
                    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
                    free(combined);
                }
                break;
            }
            case HASS_DEV_TYP_climate: {
                int thermostat_nr;
                if ((thermostat_nr = poolstate_thermostat_nr(dispatch->dev_name)) >= 0) {
                    uint8_t const heat_src = state->thermostats[dispatch->dev_idx].heat_src;
                    uint8_t const target_temp = state->thermostats[dispatch->dev_idx].set_point;
                    uint8_t const current_temp = state->thermostats[dispatch->dev_idx].temp;
                    char * combined;
                    assert( asprintf(&combined, "homeassistant/%s/%s/%s/state"
                                     "\t"
                                     "{"
                                     "\"mode\":\"%s\","
                                     "\"target_temp\":\"%u\","
                                     "\"current_temp\":\"%u\"}",
                                     hass_dev_typ_str(dispatch->dev_type), ipc->dev.name, dispatch->dev_name, network_heat_src_str(heat_src), target_temp, current_temp) >= 0 );
                    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_PUBLISH, combined, ipc);
                    free(combined);
                } else {
                    ESP_LOGE(TAG, "thermostat not found (%s)", dispatch->dev_name);
                }
            }
        }  // switch
    }  // for

    size_t json_size = 1024;
    char json[json_size];
    poolstate_to_json(state, json, json_size);
    ipc_send_to_mqtt(IPC_TO_MQTT_TYP_STATE, json, ipc);
    return ESP_OK;
}
