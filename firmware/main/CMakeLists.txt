idf_component_register(
    SRCS "logger.c"
    INCLUDE_DIRS "." "../include"
    REQUIRES dht esp_wifi esp_http_client nvs_flash
)
