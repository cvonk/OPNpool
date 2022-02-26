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

esp_err_t
httpd_root(httpd_req_t * const req)
{
    if (req->method != HTTP_GET) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "No such method");
        return ESP_FAIL;
    }

    //const char * resp_str = (const char *) req->user_ctx;  // send string passed in user context as response
//#define LOCALHTML (1)
#ifdef LOCALHTML    
    static const char * resp_str = "<html><head> <meta charset='utf-8'/> <title>Pool webapp</title> <link rel='shortcut icon' href='//coertvonk.com/wp-content/themes/atahualpa/images/favicon/coert.ico' type='image/x-icon'/> <script src='https://ajax.googleapis.com/ajax/libs/jquery/2.1.4/jquery.min.js'></script> <link rel='stylesheet' href='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css'> <script src='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js'></script><!-- OLD <script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script> <link rel='stylesheet' href='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css'> <script src='https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js'></script>--> <link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/round-slider@1.6.1/dist/roundslider.min.css'> <script src='https://cdn.jsdelivr.net/npm/round-slider@1.6.1/dist/roundslider.min.js'></script> <link rel='stylesheet' href='https://coertvonk.com/cvonk/pool/index.css'></head><body> <div data-role='page' data-theme='b' data-content-theme='b'> <div data-role='header' data-theme='a' data-content-theme='b'> <h1>Pool</h1> </div><div data-role='content'> <div id='colborders' data-role='collapsibleset' data-iconpos='right'> <div id='circuits_collapsible' data-role='collapsible'> <h3>Circuits</h3> <label id='circuits_lbl'></label> <div class='ui-grid-b ui-responsive'> <div class='ui-block-a center-wrapper'> <form id='circuit'> <fieldset data-role='controlgroup' data-type='horizontal' data-mini='true'> <input id='circuit_pool' type='checkbox' class='circuit' value='pool_circuit'/> <label for='circuit_pool'>Pool</label> <input id='circuit_spa' type='checkbox' class='circuit' value='spa_circuit'/> <label for='circuit_spa'>Spa</label> </fieldset> </form> </div><div class='ui-block-b center-wrapper'> <form id='circuit'> <fieldset data-role='controlgroup' data-type='horizontal' data-mini='true'> <input id='circuit_aux1' type='checkbox' class='circuit' value='aux1_circuit'/> <label for='circuit_aux1'>Aux1</label> <input id='circuit_aux2' type='checkbox' class='circuit' value='aux2_circuit'/> <label for='circuit_aux2'>Aux2</label> <input id='circuit_aux3' type='checkbox' class='circuit' value='aux3_circuit'/> <label for='circuit_aux3'>Aux3</label> </fieldset> </form> </div><div class='ui-block-c center-wrapper'> <form id='circuit'> <fieldset data-role='controlgroup' data-type='horizontal' data-mini='true'> <input id='circuit_ft1' type='checkbox' class='circuit' value='ft1_circuit'/> <label for='circuit_ft1'>Ft1</label> <input id='circuit_ft2' type='checkbox' class='circuit' value='ft2_circuit'/> <label for='circuit_ft2'>Ft2</label> <input id='circuit_ft3' type='checkbox' class='circuit' value='ft3_circuit'/> <label for='circuit_ft3'>Ft3</label> <input id='circuit_ft4' type='checkbox' class='circuit' value='ft4_circuit'/> <label for='circuit_ft4'>Ft4</label> </fieldset> </form> </div></div></div><!-- <div data-role='collapsible'> <h3>Solar temp.</h3> <label id='temp_solar_lbl'></label> <div class='ui-grid-a ui-responsive'> <div class='ui-block-solo center-wrapper'> <div id='temp_solar'></div></div></div></div>--> <div id='temp_water_collapsible' data-role='collapsible'> <h3>Water temperature</h3> <label id='temp_water_lbl'></label> <div class='ui-grid-a ui-responsive'> <div class='ui-block-solo center-wrapper'> <div id='temp_water'></div></div></div></div><div id='temp_air_collapsible' data-role='collapsible'> <h3>Air temperature</h3> <label id='temp_air_lbl'></label> <div class='ui-grid-a ui-responsive'> <div class='ui-block-solo center-wrapper'> <div id='temp_air'></div></div></div></div><div id='thermo_pool_collapsible' data-role='collapsible'> <h3>Pool thermostat</h3> <label id='thermo_pool_lbl'></label> <div class='ui-grid-a ui-responsive'> <div class='ui-block-a center-wrapper'> <div id='thermo_pool'></div></div><div class='ui-block-b center-wrapper'> <form id='pool_heater'> <fieldset data-role='controlgroup' data-type='horizontal' data-mini='true'> <input id='pool_heater_off' class='heat_src pool_heater' value='off' type='radio' name='pool_heater'/> <label for='pool_heater_off'>Off</label> <input id='pool_heater_heat' class='heat_src pool_heater' value='heat' type='radio' name='pool_heater'/> <label for='pool_heater_heat'>Heater</label> </fieldset> </form> <form> <fieldset data-role='controlgroup' data-type='horizontal' data-mini='true'> <input id='pool_heater_solar_pref' class='heat_src pool_heater' value='solar_pref' type='radio' name='pool_heater'/> <label for='pool_heater_solar_pref'>Solar pref</label> <input id='pool_heater_solar' class='heat_src pool_heater' value='solar' type='radio' name='pool_heater'/> <label for='pool_heater_solar'>Solar</label> </fieldset> </form> </div></div></div><div id='thermo_spa_collapsible' data-role='collapsible'> <h3>Spa thermostat</h3> <label id='thermo_spa_lbl'></label> <div class='ui-grid-a ui-responsive'> <div class='ui-block-a center-wrapper'> <div id='thermo_spa'></div></div><div class='ui-block-b center-wrapper'> <form id='spa_heater'> <fieldset data-role='controlgroup' data-type='horizontal' data-mini='true'> <input id='spa_heater_off' class='heat_src spa_heater' value='off' type='radio' name='spa_heater'/> <label for='spa_heater_off'>Off</label> <input id='spa_heater_heat' class='heat_src spa_heater' value='heat' type='radio' name='spa_heater'/> <label for='spa_heater_heat'>Heater</label> </fieldset> </form> <form> <fieldset data-role='controlgroup' data-type='horizontal' data-mini='true'> <input id='spa_heater_solar_pref' class='heat_src spa_heater' value='solar_pref' type='radio' name='spa_heater'/> <label for='spa_heater_solar_pref'>Solar pref</label> <input id='spa_heater_solar' class='heat_src spa_heater' value='solar' type='radio' name='spa_heater'/> <label for='spa_heater_solar'>Solar</label> </fieldset> </form> </div></div></div><div id='pump_rpm_collapsible' data-role='collapsible'> <h3>Pump speed</h3> <label id='pump_rpm_lbl'></label> <div class='ui-grid-a ui-responsive'> <div class='ui-block-solo center-wrapper'> <div id='pump_rpm'></div></div></div></div><!-- <div id='pump_pwr_collapsible' data-role='collapsible'> <h3>Pump power</h3> <label id='pump_pwr_lbl'></label> <div class='ui-grid-a ui-responsive'> <div class='ui-block-solo center-wrapper'> <div id='pump_pwr'></div></div></div></div>--> <div id='chlor_salt_collapsible' data-role='collapsible'> <h3>Salt level</h3> <label id='chlor_salt_lbl'></label> <div class='ui-grid-a ui-responsive'> <div class='ui-block-solo center-wrapper'> <div id='chlor_salt'></div></div></div></div><div id='chlor_pct_collapsible' data-role='collapsible'> <h3>Chlorinator</h3> <label id='chlor_pct_lbl'></label> <div class='ui-grid-a ui-responsive'> <div class='ui-block-solo center-wrapper'> <div id='chlor_pct'></div></div></div></div><div id='system_collapsible' data-role='collapsible'> <h3>System</h3> <div class='ui-body ui-grid-a ui-responsive'> <div class='ui-block-a'> <input id='date' type='date' disabled/> </div><div class='ui-block-b'> <input id='time' type='time' disabled/> </div></div></div></div></div></div><script src='https://coertvonk.com/cvonk/pool/index.js'></script> </body></html>";
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
