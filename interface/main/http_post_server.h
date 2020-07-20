#pragma once
#include <esp_http_server.h>
#include "ipc_msgs.h"

void http_post_server_stop(httpd_handle_t server);
httpd_handle_t http_post_server_start(ipc_t const * const ipc);
