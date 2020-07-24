set(COMPONENT_SRCS main.c
                   rs485/rs485.c
                   datalink/datalink.c datalink/datalink_str.c
                   network/network.c network/receive_msg.c network/transmit_msg.c network/network_str.c
                   poolstate/poolstate.c poolstate/receive_update.c poolstate/to_json.c
                   httpd/httpd.c httpd/httpd_root.c httpd/httpd_json.c httpd/httpd_api.c
                   pool_task.c
                   mqtt/mqtt_task.c
                   utils/board_name.c utils/utils_str.c
                   )
set(COMPONENT_ADD_INCLUDEDIRS .)
register_component()
