/* MDNS-SD Advertise
 *
 * Based on PD/CC0 https://github.com/espressif/esp-idf/blob/ad3b820e701c3ef0803b045b5a2c5ef19630fb0b/examples/protocols/mdns/main/mdns_example_main.c
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "driver/gpio.h"
#include <sys/socket.h>
#include <netdb.h>
#include <esp_ota_ops.h>

#include "pool_mdns.h"

static const char * TAG = "pool_mdns";

#define MDNS_HOSTNAME "pool"
#define MDNS_INSTANCE "Pool Interface"
#define MDNS_ADD_MAC_TO_HOSTNAME y

/** Generate host name based on sdkconfig, optionally adding a portion of MAC address to it.
 *  @return host name string allocated from the heap
 */
static char * generate_hostname()
{
#ifndef MDNS_ADD_MAC_TO_HOSTNAME
    return strdup(MDNS_HOSTNAME);
#else
    uint8_t mac[6];
    char * hostname;
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (-1 == asprintf(&hostname, "%s-%02X%02X%02X", MDNS_HOSTNAME, mac[3], mac[4], mac[5])) {
        abort();
    }
    return hostname;
#endif
}

void pool_mdns_init(void)
{
    char * hostname = generate_hostname();
    {
        ESP_ERROR_CHECK( mdns_init() );
        ESP_ERROR_CHECK( mdns_hostname_set(MDNS_HOSTNAME) );  // set mDNS hostname (required if you want to advertise services)
        ESP_LOGI(TAG, "mDNS hostname [%s]", MDNS_HOSTNAME);
        ESP_ERROR_CHECK( mdns_instance_name_set(MDNS_INSTANCE) );  //set default mDNS instance name

        esp_app_desc_t const * app_description = esp_ota_get_app_description();
        if (app_description) {
            mdns_txt_item_t serviceTxtData[] = {
                {"name", (char *)app_description->project_name},
                {"version", (char *)app_description->version},
                {"date", (char *)app_description->date},
                {"time", (char *)app_description->time}
            };
/*
 (64751) pool_mdns: mDNS hostname [pool]
ESP_ERROR_CHECK failed: esp_err_t 0x102 (ESP_ERR_INVALID_ARG) at 0x40091864
0x40091864: _esp_error_check_failed at C:/Users/coert/espressif/esp-idf/components/esp32/panic.c:726

file: "../components/pool-mdns/pool_mdns.c" line 64
func: pool_mdns_iI (6n475it
e1)xpression: mdns_service_add("Pool Interface", "_http", "_t pool_cp", 80, serviceTxtData, sizeof(serviceTxtData)/sizeof(servota: Running partitioiceTxtData[0]))iceTxtData[0]))
n factory at offset 0x00010000
*/
            ESP_ERROR_CHECK( mdns_service_add("Pool Interface", "_http", "_tcp", 80, serviceTxtData,
                sizeof(serviceTxtData)/sizeof(serviceTxtData[0])) );
        } else {
            ESP_LOGE(TAG, "app descr failed");
        }
    }
    free(hostname);
}
