/**
 * @brief OPNpool - HTTPd: started in response to receiving an IP address; http_uries to httpd_*.c callbacks
 *
 * © Copyright 2014, 2019, 2022, Coert Vonk
 * 
 * This file is part of OPNpool.
 * OPNpool is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * OPNpool is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with OPNpool. 
 * If not, see <https://www.gnu.org/licenses/>.
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: Copyright 2014,2019,2022 Coert Vonk
 */

#include <string.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <mdns.h>

#include "httpd.h"
#include "../ipc/ipc.h"

static char const * const TAG = "httpd";

static httpd_uri_t _httpd_uris[] = {
    { .uri = "/",            .method = HTTP_GET, .handler = httpd_root },  // httpd_root.c
    { .uri = "/json",        .method = HTTP_GET, .handler = httpd_json },  // httpd_json.c
    { .uri = "/who",        . method = HTTP_GET, .handler = httpd_who  },  // httpd_who.c
    { .uri = "/favicon.ico", .method = HTTP_GET, .handler = httpd_ico  },  // httpd_ico.c
};

/*
 * Register the URI handlers for incoming GET requests.
 */

void
httpd_register_handlers(httpd_handle_t const httpd_handle, esp_ip4_addr_t const * const ip, ipc_t const * const ipc)
{
    httpd_uri_t * http_uri = _httpd_uris;
    for (int ii = 0; ii < ARRAY_SIZE(_httpd_uris); ii++, http_uri++) {

        http_uri->user_ctx = (void *) ipc;
        ESP_ERROR_CHECK( httpd_register_uri_handler(httpd_handle, http_uri) );
        if (CONFIG_OPNPOOL_DBGLVL_HTTPD > 1) {
            ESP_LOGI(TAG, "Listening at http://" IPSTR "%s", IP2STR(ip), http_uri->uri);
        }
    }

	// mDNS

	ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("opnpool"));
    ESP_ERROR_CHECK(mdns_instance_name_set("OPNpool interface"));
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    ESP_ERROR_CHECK(mdns_service_instance_name_set("_http", "_tcp", "OPNpool"));
}

/*
 * Decodes a URL string
 * Caller MUST free alloc'ed mem.
 * From https://github.com/wcharczuk/urlencode
 */

char *
httpd_urldecode(char const * in)
{
	size_t const in_len = strlen(in);
	size_t out_len = (in_len + 1) * sizeof(char);
	char *p = malloc(out_len), *out = p;

	while (*in) {
		if (*in == '%') {
			char const buffer[3] = { in[1], in[2], 0 };
			*p++ = strtol(buffer, NULL, 16);
			in += 3;
		} else {
			*p++ = *in++;
		}
	}
	*p = '\0';
	return out;
}
