#pragma once
#include <esp_netif.h>

typedef void (* wifi_connect_on_connect_t)(void * const priv, esp_ip4_addr_t const * const ip);
typedef void (* wifi_connect_on_disconnect_t)(void * const priv);

typedef struct wifi_connect_config_t {
    wifi_connect_on_connect_t     onConnect;     // called when WiFi is connected (e.g. to start an http server)
    wifi_connect_on_disconnect_t  onDisconnect;  // called when WiFi is disconnected (e.g. to close an http server)
    void *                        priv;          // pointer to data specific to requester
} wifi_connect_config_t;

esp_err_t wifi_connect(wifi_connect_config_t * const config);