#pragma once
#include <sdkconfig.h>
#include <esp_system.h>

// for ic messages, the datalink relies on the specific pkt len for each ic_typ
uint8_t network_ic_len(uint8_t const ic_typ);
