idf_component_register(SRCS "adc.c" "mqtt.c" "app_main.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif
                    REQUIRES esp_adc
                    INCLUDE_DIRS ".")
