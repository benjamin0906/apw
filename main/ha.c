#include "mqtt.h"
#include "stdint.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "adc.h"

typedef enum 
{
    HA_StartMqtt,
    HA_WaitMqtt,
    HA_SendDiscovery,
    HA_Running,
} dtHA_States;

static char* cfg_topic = "homeassistant/sensor/voltage/config";
static char discovery_msg[] = "{\"name\":\"esp32_voltage\",\"unique_id\":\"esp32_my_test_device\",\"state_topic\":\"my_esp32/voltage\",\"unit_of_measurement\":\"V\",\"device\":{\"identifiers\":\"[my_esp32_volt_meter]\",\"name\":\"volt_meter\"}}";
static dtHA_States state;
static const uint8_t broker_url[] = CONFIG_BROKER_URL;
static const char TAG[] = "HA";

void HA_Runnable(void)
{
    TickType_t start;
    TickType_t end;
    uint16_t filter[10];
    while(1)
    {
        start = xTaskGetTickCount();
        switch(state)
        {
            case HA_StartMqtt:
                mqtt_app_start(broker_url);
                state = HA_WaitMqtt;
                ESP_LOGI(TAG, "MQTT has started");
                break;
            case HA_WaitMqtt:
                if(mqtt_isConnected() != 0)
                {
                    state = HA_SendDiscovery;
                    ESP_LOGI(TAG, "MQTT has been connected");
                }
                break;
            case HA_SendDiscovery:
                if(mqtt_publish("homeassistant/sensor/voltage/config", discovery_msg, 0, 0, 0) == 0)
                {
                    state = HA_Running;
                    ESP_LOGI(TAG, "MQTT discovery message has been sent");
                }
                break;
            case HA_Running:
                {
                    uint8_t looper;
                    uint16_t value = adc_get_volt();
                    uint32_t sum = 0;
                    memcpy(&filter[0], &filter[1], sizeof(filter) - sizeof(filter[0]));
                    filter[9] = value;
                    for(looper = 0; looper < (sizeof(filter)/sizeof(filter[0])); looper++)
                    {
                        sum += filter[looper];
                    }
                    value = sum / looper;
                    ESP_LOGI(TAG, "Sum: %i, Looper: %i, Value:%i", (int)sum, (int)looper, (int)value);
                }
                break;
        }
        end = xTaskGetTickCount();
        vTaskDelay((100 / portTICK_PERIOD_MS) - ((end - start)));
    }
}