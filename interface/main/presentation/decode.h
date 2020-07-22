#pragma once

#include <esp_system.h>
#include "../datalink/datalink.h"
#include "presentation.h"

void network_decode(datalink_msg_t const * const datalink, network_msg_t * network);
