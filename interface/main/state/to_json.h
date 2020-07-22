#pragma once

#include <esp_system.h>
#include "../presentation/presentation.h"

size_t state_to_json(poolstate_t * state, char * buf, size_t buf_len);