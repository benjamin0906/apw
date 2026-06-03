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
#include "esp_wifi.h"

#include "esp_log.h"
#include "mqtt.h"
#include "adc.h"
#include "ha.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "apw";
static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

static void event_handler(void * arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < 10) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    wifi_config_t wifi_config = {
        .sta = {
        .ssid = "DB2_4",
        .password = "41302202",
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        .sae_h2e_identifier = "",
        }
    };
    EventBits_t bits;
    s_wifi_event_group = xEventGroupCreate();

    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    esp_wifi_init(&cfg);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void app_main(void)
{
    char broker[] = CONFIG_BROKER_URL;
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
    wifi_init_sta();
    xTaskCreate((TaskFunction_t)&HA_Runnable, "HA_Task", 1024, NULL, 6, NULL);

    //ESP_ERROR_CHECK(nvs_flash_init());
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    //ESP_ERROR_CHECK(example_connect());
    adc_init();
    //mqtt_app_start((uint8_t*)broker);
    //while(mqtt_isConnected() == 0);
    //while(0 != mqtt_publish("homeassistant/sensor/voltage/config", discovery_msg, 0, 0, 0));
    char data[10] = {'0', 0};
    while(1)
    {
        //HA_Runnable();
        uint16_t adc_raw = adc_get_raw();
        uint16_t adc_volt = adc_get_volt();
        data[sprintf(data, "%d", (int)adc_volt)] = 0;
        printf("adc raw: %d, voltage: %d, %s\n", adc_raw, adc_volt, data);
        //mqtt_publish("my_esp32/voltage_asd", data, 0, 0, 0);
        if(data[0] >= '9')
        {
            data[0] = '0';
        }else data[0]++;
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
