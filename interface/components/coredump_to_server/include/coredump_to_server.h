#pragma once

#include "esp_err.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "sdkconfig.h"

typedef esp_err_t (* coredump_to_server_start_t)(void *priv);
typedef esp_err_t (* coredump_to_server_end_t)(void * priv);
typedef esp_err_t (* coredump_to_server_write_t)(void *priv, char const * const str);

typedef struct coredump_to_server_config_t {
    coredump_to_server_start_t  start;  // this function is called before writin data chunks (e.g. to open connection to srv)
    coredump_to_server_end_t    end;    // this function is called when all dump data are written (e.g. to close connection to srv)
    coredump_to_server_write_t  write;  // this function is called to write data chunk
    void *                      priv;   // pointer to data specific to requester
} coredump_to_server_config_t;

esp_err_t coredump_to_server(coredump_to_server_config_t const * const cfg);
