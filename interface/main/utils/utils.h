#pragma once

#include <strings.h>
#include "../datalink/datalink.h"
#include "../network/network.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif
#ifndef ELEM_AT
# define ELEM_AT(a, i, v) ((uint) (i) < ARRAY_SIZE(a) ? (a)[(i)] : (v))
#endif
#ifndef ELEM_POS
# define ELEM_POS(a, s) \
    do { \
      for (uint_least8_t ii = 0; ii < ARRAY_SIZE(a); ii++) { \
	    if (strcasecmp(s, a[ii]) == 0) { \
	      return ii; \
	    } \
      } \
      return -1; \
    } while(0)
#endif

// also used to display date/time, add +50
#define NAME_BUF_SIZE ((sizeof(datalink_hdr_t) + sizeof(network_msg_ctrl_state_t) + 1) * 3 + 50)

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

/* utils_str.c */
void name_reset_idx(void);
char const * hex8_str(uint8_t const value);
char const * hex16_str(uint16_t const value);
extern name_str_t name_str;

/* board_name.c */
void board_name(char * const name, size_t name_len);

/* strl.c */
size_t strlcpy(char * __restrict dst, const char * __restrict src, size_t dsize);
size_t strlcat(char *dst, const char *src, size_t siz);
