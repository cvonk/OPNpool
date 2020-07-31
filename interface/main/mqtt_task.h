#pragma once

#if !(defined CONFIG_POOL_DBG_MQTTTASK)
#  define CONFIG_POOL_DBG_MQTTTASK (0)
#endif
#if !(defined CONFIG_POOL_DBG_MQTTTASK_ONERROR)
#  define CONFIG_POOL_DBG_MQTTTASK_ONERROR (0)
#endif

void mqtt_task(void * ipc_void);

