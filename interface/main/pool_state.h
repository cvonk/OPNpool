#pragma once

typedef struct pool_state_t {
    bool dummy;
} pool_state_t;

void pool_state_init(void);
void pool_state_set(pool_state_t const * const state);;
void pool_state_get(pool_state_t * const state);
