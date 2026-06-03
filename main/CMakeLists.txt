idf_component_register(SRCS "ha.c" "adc.c" "mqtt.c" "app_main.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif
                    REQUIRES esp_adc esp_wifi
                    INCLUDE_DIRS ".")
