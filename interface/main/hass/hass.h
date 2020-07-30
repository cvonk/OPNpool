#pragma once
#include <esp_system.h>

#include "../ipc/ipc.h"

void hass_init(ipc_t const * const ipc);
esp_err_t hass_rx_mqtt(char * const topic, char const * const value_str, datalink_pkt_t * const pkt);
esp_err_t hass_tx_state(poolstate_t const * const state, ipc_t const * const ipc);