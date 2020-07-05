set(COMPONENT_SRCS "protocomm_handlers.c" "main.c" "ble_prov.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(REQUIRES pool_ota)
register_component()