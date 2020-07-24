#pragma once
#include <esp_http_server.h>

/* httpd.c */
esp_err_t httpd_cb(httpd_req_t * const req);
char const * httpd_get_content(httpd_req_t * const req) ;

/* httpd_root.c */
esp_err_t httpd_root(httpd_req_t * const req);

/* httpd_json.c */
esp_err_t httpd_json(httpd_req_t * const req);

/* httpd_api.c */
esp_err_t httpd_api(httpd_req_t * const req);
