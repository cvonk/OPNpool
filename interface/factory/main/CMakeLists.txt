set(COMPONENT_SRCS "main.c" "ble_prov.c" "ble_prov_handlers.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(REQUIRES pool_ota)
register_component()