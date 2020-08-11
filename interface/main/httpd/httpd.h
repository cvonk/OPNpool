#pragma once
#include <esp_wifi.h>
#include <esp_http_server.h>

#include "../ipc/ipc.h"

/* httpd.c */
void httpd_register_cb(httpd_handle_t const httpd_handle, esp_ip4_addr_t const * const ip, ipc_t const * const ipc);
char * httpd_urldecode(char const * const in);

/* httpd_root.c */
esp_err_t httpd_root(httpd_req_t * const req);

/* httpd_json.c */
esp_err_t httpd_json(httpd_req_t * const req);

/* httpd_ico.c */
esp_err_t httpd_ico(httpd_req_t * const req);