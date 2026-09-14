idf_component_register(SRCS "wifi.c" "ha.c" "adc.c" "mqtt.c" "app_main.c" "watering_control.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif esp_wifi esp_driver_gpio esp_driver_gptimer esp_timer
                    REQUIRES esp_adc esp_wifi esp_driver_gpio esp_driver_gptimer esp_timer esp_pm
                    INCLUDE_DIRS ".")
