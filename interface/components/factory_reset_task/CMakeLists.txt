set(COMPONENT_SRCS src/factory_reset_task.c)
set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_REQUIRES nvs_flash app_update driver)
register_component()
