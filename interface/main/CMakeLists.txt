set(COMPONENT_SRCS main.c mqtt/mqtt_task.c utils/board_name.c httpd/httpd.c httpd/httpd_root.c httpd/httpd_json.c httpd/httpd_api.c datalink/rs485.c datalink/packetizer_task.c state/poolstate.c proto/name.c state/to_json.c)
set(COMPONENT_ADD_INCLUDEDIRS .)
register_component()
