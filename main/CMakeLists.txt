idf_component_register(SRCS "http_server.c" "dtos.c" "main.c" "net_app.c" "http_server.c" "dtos.c"
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage web FLASH_IN_PROJECT)