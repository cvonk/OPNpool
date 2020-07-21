#pragma once
#include <esp_http_server.h>

esp_err_t httpd_cb(httpd_req_t * const req);
char const * httpd_get_content(httpd_req_t * const req) ;
