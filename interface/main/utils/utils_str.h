#pragma once

#include "../datalink/datalink.h"
#include "../network/network.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif
#ifndef ELEM_AT
# define ELEM_AT(a, i, v) ((uint) (i) < ARRAY_SIZE(a) ? (a)[(i)] : (v))
#endif

// also used to display date/time, add +50
#define NAME_BUF_SIZE ((sizeof(datalink_hdr_t) + sizeof(mCtrlState_a5_t) + 1) * 3 + 50)

// reusable global string
typedef struct str_t {
	char str[NAME_BUF_SIZE];  // 3 bytes for each hex value when displaying raw; this is str.str[]
	uint_least8_t idx;
	char const * const noMem;
	char const * const digits;
} name_str_t;

typedef struct str_value_name_pair_t {
    uint typ;
    char const * const str;
} str_value_name_pair_t;

void name_reset_idx(void);
char const * hex8_str(uint8_t const value);
char const * hex16_str(uint16_t const value);

extern name_str_t name_str;