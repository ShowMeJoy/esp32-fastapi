idf_component_register(
    SRCS "logger.c"
    INCLUDE_DIRS "." "../include"
    REQUIRES bmp180 dht esp_wifi esp_http_client nvs_flash
)
