
set(COMPONENT_SRCS src/led_strip_rmt_ws2812.c)
set(COMPONENT_ADD_INCLUDEDIRS include)
#set(COMPONENT_PRIV_REQUIRES rmt)
set(COMPONENT_PRIV_REQUIRES driver)
register_component()
