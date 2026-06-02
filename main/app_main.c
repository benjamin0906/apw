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
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include "mqtt.h"
#include "adc.h"

static const char *TAG = "apw";



void app_main(void)
{
    char discovery_msg[] = "{\"name\":\"esp32_voltage\",\"unique_id\":\"esp32_my_test_device\",\"state_topic\":\"my_esp32/voltage\",\"unit_of_measurement\":\"V\",\"device\":{\"identifiers\":\"[my_esp32_volt_meter]\",\"name\":\"volt_meter\"}}";
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("apw", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    adc_init();
    mqtt_app_start();
    while(mqtt_isConnected() == 0);
    while(0 != mqtt_publish("homeassistant/sensor/voltage/config", discovery_msg, 0, 0, 0));
    char data[10] = {'0', 0};
    while(1)
    {
        uint16_t adc_raw = adc_get_raw();
        uint16_t adc_volt = adc_get_volt();
        data[sprintf(data, "%d", (int)adc_volt)] = 0;
        printf("adc raw: %d, voltage: %d, %s", adc_raw, adc_volt, data);
        mqtt_publish("my_esp32/voltage", data, 0, 0, 0);
        if(data[0] >= '9')
        {
            data[0] = '0';
        }else data[0]++;
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
