/**
 * @brief Pool state and access functions
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2015 - 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <stdio.h>
#include <string.h>
#include <sdkconfig.h>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "pool_state.h"

static struct pool_state_prot {
    SemaphoreHandle_t xMutex;
    pool_state_t * state;
} _protected;

void
pool_state_set(pool_state_t const * const state)
{
    xSemaphoreTake( _protected.xMutex, portMAX_DELAY );
    {
        memcpy(_protected.state, state, sizeof(pool_state_t));
    }
    xSemaphoreGive( _protected.xMutex );
}

void
pool_state_get(pool_state_t * const state)
{
    xSemaphoreTake( _protected.xMutex, portMAX_DELAY );
    {
        memcpy(state, _protected.state, sizeof(pool_state_t));
    }
    xSemaphoreGive( _protected.xMutex );
}

void
pool_state_init(void)
{
    _protected.xMutex = xSemaphoreCreateMutex();
    _protected.state = malloc(sizeof(pool_state_t));
    assert(_protected.xMutex && _protected.state);
}