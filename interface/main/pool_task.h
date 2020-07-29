#pragma once
#include <sdkconfig.h>
#include <esp_system.h>

#if !(defined CONFIG_POOL_DBG_POOLTASK)
#  define CONFIG_POOL_DBG_POOLTASK (0)
#endif
#if !(defined CONFIG_POOL_DBG_POOLTASK_ONERROR)
#  define CONFIG_POOL_DBG_POOLTASK_ONERROR (0)
#endif

void pool_task(void * ipc_void);
