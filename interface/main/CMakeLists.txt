set(COMPONENT_SRCS main.c mqtt/mqtt_task.c utils/board_name.c httpd/httpd.c httpd/httpd_root.c httpd/httpd_json.c httpd/httpd_api.c datalink/rs485.c datalink/datalink.c state/poolstate.c presentation/presentation.c presentation/name.c presentation/decode.c presentation/encode.c state/to_json.c)
set(COMPONENT_ADD_INCLUDEDIRS .)
register_component()
