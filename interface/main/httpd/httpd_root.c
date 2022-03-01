/**
 * @brief HTTPd: HTTP server callback for endpoint "/"
 *
 * CLOSED SOURCE, NOT FOR PUBLIC RELEASE
 * (c) Copyright 2020-2022, Coert Vonk
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
#include "../ipc/ipc.h"

//static char const * const TAG = "httpd_root";

/*
 * URI handler for incoming GET "/" requests.
 * Registered in `httpd_register_handlers`
 */

esp_err_t
httpd_root(httpd_req_t * const req)
{
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    //const char * resp_str = (const char *) req->user_ctx;  // send string passed in user context as response
#define LOCALHTML (1)
#ifdef LOCALHTML    
    static const char * resp_str = "<html>\n
        "<head>\n"
        "  <meta charset='utf-8'/>\n<title>Pool webapp</title></meta>\n"
        "  <link rel='shortcut icon' href='//coertvonk.com/wp-content/themes/atahualpa/images/favicon/coert.ico' type='image/x-icon'/>\n"
        "  <script src='https://ajax.googleapis.com/ajax/libs/jquery/2.1.4/jquery.min.js'></script>\n"
        "  <link rel='stylesheet' href='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css'>\n"
        "  <script src='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js'></script>\n"
        "  <link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/round-slider@1.6.1/dist/roundslider.min.css'>\n"
        "  <script src='https://cdn.jsdelivr.net/npm/round-slider@1.6.1/dist/roundslider.min.js'></script>\n"
        "  <link rel='stylesheet' href='https://coertvonk.com/cvonk/pool/index.css'>\n"
        "</head>\n"
        "<body>\n"
        "  <style>\n"
        "    body {width:100%;height:100%;margin:0;overflow:hidden;background-color:#252525;}\n"
        "  </style>\n"
        "  <h1>Page loading</h1>\n"
        "</body>\n"
        "</html>";
#else
    const char * resp_str = "<html>\n"
        "<head>\n"
        "  <meta name='viewport' content='width=device-width, initial-scale=1, minimum-scale=1.0 maximum-scale=1.0' />\n"
        "  <script type='text/javascript'>\n"
        "    window.addEventListener('message', event => {\n"
        "      var iframe = document.getElementsByTagName('iframe')[0].contentWindow;\n"
        "      iframe.postMessage('toIframe', event.origin);\n"
        "    });\n"
        "  </script>\n"
        "</head>\n"
        "<body>\n"
        "  <style>\n"
        "    body {width:100%;height:100%;margin:0;overflow:hidden;background-color:#252525;}\n"
        "    #iframe {position:absolute;left:0px;top:0px;}\n"
        "  </style>\n"
        "  <h1>Page loading</h1>\n"
        "  <iframe id='iframe' name='iframe1' frameborder='0' width='100%' height='100%' src='//coertvonk.com/cvonk/pool/'></iframe>\n"
        "</body>\n"
        "</html>";
#endif
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}
