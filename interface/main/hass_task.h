#pragma once
#include <esp_system.h>

#include "ipc/ipc.h"
#include "poolstate/poolstate.h"

#if !(defined CONFIG_POOL_DBG_HASSTASK)
#  define CONFIG_POOL_DBG_HASSTASK (0)
#endif
#if !(defined CONFIG_POOL_DBG_HASSTASK_ONERROR)
#  define CONFIG_POOL_DBG_HASSTASK_ONERROR (0)
#endif

#define HASS_DEV_TYP_MAP(XX) \
  XX(0, switch) \
  XX(1, sensor) \
  XX(2, climate)

typedef enum {
#define XX(num, name) HASS_DEV_TYP_##name = num,
  HASS_DEV_TYP_MAP(XX)
#undef XX
} hass_dev_typ_t;

/* hass_task.c */
void hass_task(void * ipc_void);
esp_err_t hass_rx_mqtt(char * const topic, char const * const value_str, datalink_pkt_t * const pkt);
esp_err_t hass_tx_state(poolstate_t const * const state, ipc_t const * const ipc);

/* hass_str.c */
char const * hass_dev_typ_str(hass_dev_typ_t const dev_typ);
