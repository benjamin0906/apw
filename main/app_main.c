/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_system.h"
#include "esp_event.h"

#include "esp_log.h"
#include "mqtt.h"
#include "adc.h"
#include "ha.h"
#include "driver/gpio.h"
#include "wifi.h"
#include "watering_control.h"
#include "esp_pm.h"
#include "esp_timer.h"

static const char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);
    esp_pm_config_t pm_config = {.max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ, .min_freq_mhz = 10, .light_sleep_enable = true};
    //ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
    WIFI_Init();
    WIFI_Start();
    while(WIFI_IsConnected() == 0)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    HA_Enable(1);
    //xTaskCreate((TaskFunction_t)&HA_Runnable, "HA_Task", 2048, NULL, 6, NULL);
    //xTaskCreate((TaskFunction_t)&WC_Runnable, "HA_Task", 2048, NULL, 6, NULL);

        gpio_config_t pin_led = {
    .intr_type = GPIO_INTR_DISABLE, 
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = 1ULL << GPIO_NUM_8,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE};
    gpio_config(&pin_led);
    gpio_sleep_sel_dis(GPIO_NUM_8);

    uint8_t counter = 0;
    while(1)
    {
        if(counter >= 10)
        {
            printf("alive %lu\n", (uint32_t)esp_timer_get_time());
            /*gpio_set_drive_capability(GPIO_NUM_12, GPIO_DRIVE_CAP_3);
            gpio_set_drive_capability(GPIO_NUM_13, GPIO_DRIVE_CAP_3);
            gpio_set_drive_capability(GPIO_NUM_14, GPIO_DRIVE_CAP_3);
            gpio_set_drive_capability(GPIO_NUM_15, GPIO_DRIVE_CAP_3);
            gpio_set_drive_capability(GPIO_NUM_16, GPIO_DRIVE_CAP_3);
            gpio_set_drive_capability(GPIO_NUM_17, GPIO_DRIVE_CAP_3);*/
            counter = 0;
        }
        counter++;

        if(counter & 1)
        {
            gpio_set_level(GPIO_NUM_8, 1);
        }
        else
        {
            gpio_set_level(GPIO_NUM_8, 0);
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
