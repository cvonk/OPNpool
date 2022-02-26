idf_component_register( SRCS
                            main.c
                            rs485/rs485.c
                            datalink/datalink.c datalink/datalink_rx.c datalink/datalink_tx.c datalink/datalink_str.c
                            network/network.c network/network_rx.c network/network_tx.c network/network_str.c
                            poolstate/poolstate.c poolstate/poolstate_rx.c poolstate/poolstate_json.c poolstate/poolstate_get.c poolstate/poolstate_str.c
                            httpd/httpd.c httpd/httpd_root.c httpd/httpd_json.c httpd/httpd_ico.c
                            utils/board_name.c utils/utils_str.c utils/strl.c
                            ipc/ipc.c
                            skb/skb.c
                            hass_task.c hass_str.c
                            pool_task.c mqtt_task.c
                        INCLUDE_DIRS
                            "."
                            "../components/coredump_to_server/include"
                            "../components/factory_reset_task/include"
                            "../components/ota_update_task/include"
                            "../components/wifi_connect/include"
                            #"../components/esp-arduinojson/ArduinoJson-5.x/"
                        PRIV_REQUIRES
                            esp_http_server 
                            esp_http_client
                            esp_https_ota
                            nvs_flash
                            app_update 
                            json
                            mqtt
                            espcoredump
                        EMBED_TXTFILES 
                            httpd/favicon.ico)
