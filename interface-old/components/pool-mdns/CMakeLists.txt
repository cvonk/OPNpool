set(COMPONENT_SRCS "pool_mdns.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES nvs_flash app_update mdns esp_http_client)
register_component()
