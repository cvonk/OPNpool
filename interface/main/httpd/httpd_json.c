/**
 * @brief HTTP endpoint "/json"
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020, Coert Vonk
 * All rights reserved. Use of copyright notice does not imply publication.
 * All text above must be included in any redistribution
 **/

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "httpd.h"
#include "httpd_json.h"
#include "ipc_msgs.h"
#include "../state/poolstate.h"
#include "../state/to_json.h"

static char const * const TAG = "httpd_json";

static char *
_jsonProcessQueryVars(httpd_req_t * req, char * const buf)
{
    char * callback = NULL;  // uses JSON-P to get around CORS
    char * rest = NULL;
    for (char * key = strtok_r(buf, "&", &rest); key != NULL; key = strtok_r(NULL, "&", &rest)) {
        char * value = strchr(key, '=');
        if (value) {
            *value++ = '\0';
            if (strcmp(key, "callback") == 0) {
                callback = (char *) malloc(strlen(value)+1);
                if (callback) {
                    strcpy(callback, value);
                }
            }
            ESP_LOGI(TAG, "query var, %s=%s", key, value);
        }
#if 0
        transmitQueue.enqueue(key, value);  // MAKE SURE that the queue copies the values, as they will be freed soon
#endif
    }
    return callback;
}

// https://github.com/freebsd/freebsd/blob/master/sys/libkern/strlcpy.c
static size_t
_strlcpy(char * __restrict dst, const char * __restrict src, size_t dsize)
{
	const char *osrc = src;
	size_t nleft = dsize;

	if (nleft != 0) {  // Copy as many bytes as will fit
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}
	if (nleft == 0) { // not enough room in dst, add NUL and traverse rest of src
		if (dsize != 0)
			*dst = '\0';
		while (*src++)
			;
	}
	return(src - osrc - 1);	 // count does not include NUL
}

// https://github.com/freebsd/freebsd/blob/master/sys/libkern/strlcat.c
static size_t
_strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	while (n-- != 0 && *d != '\0')  // Find the end of dst and adjust bytes left but don't go past end
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';
	return(dlen + (s - src));  // count does not include NUL
}

esp_err_t
httpd_json(httpd_req_t * const req)
{
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    // process query vars

    char * callback = NULL;  // uses JSON-P to get around CORS
    size_t const buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char * buf = (char *) malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            callback = _jsonProcessQueryVars(req, buf);
        }
        free(buf);
    }

    // return pool state

    char resp[450];
    resp[0] = '\0';
    if (callback) _strlcpy(resp, callback, sizeof(resp));
    if (callback) _strlcat(resp, "(", sizeof(resp));

    poolstate_t state;
    poolstate_get(&state);
    uint const len = strlen(resp);
    state_to_json(&state, resp + len, sizeof(resp) - len);

    if (callback) _strlcat(resp, ")", sizeof(resp));
    free(callback);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}
