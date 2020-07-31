set(COMPONENT_SRCS rs485/rs485.c
                   datalink/datalink.c datalink/datalink_rx.c datalink/datalink_tx.c datalink/datalink_str.c
                   network/network.c network/network_rx.c network/network_tx.c network/network_str.c
                   poolstate/poolstate.c poolstate/poolstate_rx.c poolstate/poolstate_json.c poolstate/poolstate_str.c
                   httpd/httpd.c httpd/httpd_root.c httpd/httpd_json.c httpd/httpd_api.c
                   utils/board_name.c utils/utils_str.c utils/strl.c
                   ipc/ipc.c
                   skb/skb.c
                   hass_task.c hass_str.c
                   pool_task.c mqtt_task.c
                   main.c
                   )
set(COMPONENT_ADD_INCLUDEDIRS .)
register_component()
