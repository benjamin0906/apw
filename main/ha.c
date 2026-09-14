#include "mqtt.h"
#include "stdint.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "adc.h"
#include "driver/gpio.h"
#include "watering_control.h"

typedef enum 
{
    HA_InitMqtt,
    HA_Idle,
    HA_StartMqtt,
    HA_WaitMqtt,
    HA_SendDiscovery,
    HA_Running,
    HA_Stopping
} dtHA_States;

typedef struct sDevice
{
    char *id;
    char *name;
    char *manuf;
    char *model;
} dtDevice;

typedef struct sOptions
{
    uint8_t Length;
    uint8_t **Option;
} dtOptions;


typedef struct sConfig
{
    char *entity;
    char *name;
    char *unique_id;
    const char *cmd_topic;
    void (*cmd_callback)(uint8_t *data);
    uint8_t (*state_callback)(uint8_t *data);
    void (*publish_success)(void);
    const char *state_topic;
    const char *default_state;
    char *state_class;
    char *unit_of_measurement;
    char *mode;
    char *payload_press;
    uint16_t handle_period;
    uint16_t period_counter;
    int32_t *min;
    int32_t *max;
    uint32_t *step;
    dtOptions options;
    const dtDevice *const device;
} dtConfig;

static const dtDevice Device = {
    .id = "my_watering_controller",
    .name = "Watering Controller",
    .manuf = "Benjamin",
    .model = "Benjamin's_ESP32",
};

volatile static dtHA_States state;
static const uint8_t broker_url[] = "mqtt://192.168.64.2:1883";
static const char TAG[] = "HA";
static uint8_t enable_flag;

void CreateSensor(const dtConfig *const dev, char *const config_topic, char *const discovery)
{
    const char ha_base[]            = "homeassistant/";
    const char name_string[]        = "name";
    const char cfg_string[]         = "config";
    const char unique_string[]      = "unique_id";
    const char state_topic_string[] = "state_topic";
    const char cmd_topic_string[]   = "command_topic";
    const char uom_string[]         = "unit_of_measurement";
    const char mode_str[]           = "mode";
    const char min_str[]            = "min";
    const char max_str[]            = "max";
    const char step_str[]           = "step";
    const char options_str[]        = "options";
    const char payload_press_str[]  = "payload_press";
    uint8_t length;
    uint8_t field_looper;
    const char *text_fields[] = {dev->name, dev->unique_id, dev->cmd_topic, dev->state_topic, dev->unit_of_measurement, dev->mode, dev->payload_press};
    const char *field_names[] = {name_string, unique_string, cmd_topic_string, state_topic_string, uom_string, mode_str, payload_press_str};

    if((config_topic != NULL) && (discovery != NULL))
    {
        uint16_t index = sizeof(ha_base) - 1;

        memcpy(config_topic, ha_base, sizeof(ha_base));
        if(dev->entity != nullptr)
        {
            memcpy(&config_topic[index], dev->entity, strlen(dev->entity));
            index += strlen(dev->entity);
            config_topic[index++] = '/';
        }
        memcpy(&config_topic[index], dev->name, strlen(dev->name));
        index += strlen(dev->name);
        config_topic[index++] = '/';
        memcpy(&config_topic[index], cfg_string, sizeof(cfg_string));
        index += sizeof(cfg_string);
        config_topic[index++] = 0;
        //printf("CreateSensor - topic length %i\n", index);

        index = 0;
        discovery[index++] = '{';

        for(field_looper = 0; field_looper < (sizeof(field_names)/sizeof(field_names[0])); field_looper++)
        {
            if(text_fields[field_looper] != nullptr)
            {
                discovery[index++] = '"';
                length = strlen(field_names[field_looper]);
                memcpy(&discovery[index], field_names[field_looper], length);
                index += length;
                discovery[index++] = '"';
                discovery[index++] = ':';
                discovery[index++] = '"';
                length = strlen(text_fields[field_looper]);
                memcpy(&discovery[index], text_fields[field_looper], length);
                index += length;
                discovery[index++] = '"';
                discovery[index++] = ',';
            }
        }

        if(dev->min != nullptr)
        {
            discovery[index++] = '"';
            length = strlen(min_str);
            memcpy(&discovery[index], min_str, length);
            index += length;
            discovery[index++] = '"';
            discovery[index++] = ':';
            index += sprintf(&discovery[index], "%lu", *dev->min);
            discovery[index++] = ',';
        }

        if(dev->max != nullptr)
        {
            discovery[index++] = '"';
            length = strlen(max_str);
            memcpy(&discovery[index], max_str, length);
            index += length;
            discovery[index++] = '"';
            discovery[index++] = ':';
            index += sprintf(&discovery[index], "%lu", *dev->max);
            discovery[index++] = ',';
        }

        if(dev->step != nullptr)
        {
            discovery[index++] = '"';
            length = strlen(step_str);
            memcpy(&discovery[index], step_str, length);
            index += length;
            discovery[index++] = '"';
            discovery[index++] = ':';
            index += sprintf(&discovery[index], "%lu", *dev->step);
            discovery[index++] = ',';
        }

        if((dev->options.Option != nullptr) && (dev->options.Length != 0))
        {
            uint8_t looper;
            
            discovery[index++] = '"';
            length = strlen(options_str);
            memcpy(&discovery[index], options_str, length);
            index += length;
            discovery[index++] = '"';
            discovery[index++] = ':';
            discovery[index++] = '[';

            for(looper = 0; looper < dev->options.Length; looper++)
            {
                discovery[index++] = '"';
                length = strlen((char*)dev->options.Option[looper]);
                memcpy(&discovery[index], dev->options.Option[looper], length);
                index += length;
                discovery[index++] = '"';
                discovery[index++] = ',';
            }
            index--;
            discovery[index++] = ']';
            discovery[index++] = ',';
        }

        if(dev->device != NULL)
        {
            const char id_string[] = "identifiers";
            const char manu_string[] = "manufacturer";
            const char model_string[] = "model";
            const char device_string[] = "device";
            uint8_t length = strlen(device_string);
            uint8_t comma = 0;

            //discovery[index++] = ',';
            discovery[index++] = '"';
            memcpy(&discovery[index], device_string, length);
            index += length;
            discovery[index++] = '"';
            discovery[index++] = ':';
            discovery[index++] = '{';

            /* Identifier */
            if(dev->device->id != nullptr)
            {
                discovery[index++] = '"';
                length = strlen(id_string);
                memcpy(&discovery[index], id_string, length);
                index += length;
                discovery[index++] = '"';
                discovery[index++] = ':';
                discovery[index++] = '[';
                discovery[index++] = '"';
                length = strlen(dev->device->id);
                memcpy(&discovery[index], dev->device->id, length);
                index += length;
                discovery[index++] = '"';
                discovery[index++] = ']';
                comma++;
            }

            if(dev->device->name != nullptr)
            {
                if(comma != 0)
                {
                    discovery[index++] = ',';
                }
                discovery[index++] = '"';
                length = strlen(name_string);
                memcpy(&discovery[index], name_string, length);
                index += length;
                discovery[index++] = '"';
                discovery[index++] = ':';
                discovery[index++] = '"';
                length = strlen(dev->device->name);
                memcpy(&discovery[index], dev->device->name, length);
                index += length;
                discovery[index++] = '"';
                comma++;
            }

            if(dev->device->manuf != nullptr)
            {
                if(comma != 0)
                {
                    discovery[index++] = ',';
                }
                discovery[index++] = '"';
                length = strlen(manu_string);
                memcpy(&discovery[index], manu_string, length);
                index += length;
                discovery[index++] = '"';
                discovery[index++] = ':';
                discovery[index++] = '"';
                length = strlen(dev->device->manuf);
                memcpy(&discovery[index], dev->device->manuf, length);
                index += length;
                discovery[index++] = '"';
                comma++;
            }
            if(dev->device->model != nullptr)
            {
                if(comma != 0)
                {
                    discovery[index++] = ',';
                }
                discovery[index++] = '"';
                length = strlen(model_string);
                memcpy(&discovery[index], model_string, length);
                index += length;
                discovery[index++] = '"';
                discovery[index++] = ':';
                discovery[index++] = '"';
                length = strlen(dev->device->model);
                memcpy(&discovery[index], dev->device->model, length);
                index += length;
                discovery[index++] = '"';
                comma++;
            }
            discovery[index++] = '}';
            discovery[index++] = ',';
        }
        index--; //to overwrite the last comma
        discovery[index++] = '}';
        discovery[index++] = 0;
        //printf("CreateSensor - msg length %i\n", index);
    }
    //{"name":"esp32_voltage_asd","unique_id":"esp32_my_test_device_asd","state_topic":"my_esp32/voltage_asd","unit_of_measurement":"mV","device":{"identifiers":"[my_esp32_volt_meter]","name":"volt_meter"}}
}

extern uint32_t PulseSum;
extern uint32_t PulseOffset;
void HA_Runnable(void)
{
    TickType_t start;
    TickType_t end;
    char *options[] = {"0","1","2","3","4","5","6","7","8"};
    dtConfig cfg1 = {
        .entity = "select",
        .name = "watering_control",
        .unique_id = "water_ctrl",
        .cmd_topic = "water_ctrl/cmd",
        .state_topic = "water_ctrl/state",
        .unit_of_measurement = nullptr,
        .state_callback = &WC_StateCallback,
        .cmd_callback = &WC_CmdCallback,
        .handle_period = 10,
        .default_state = nullptr,
        .payload_press = nullptr,
        .period_counter = 0,
        .publish_success = nullptr,
        .options.Option = (uint8_t**)&options,
        .options.Length = sizeof(options)/sizeof(options[0]),
        .mode = nullptr,
        .min = 0,
        .max = 0,
        .step = 0,
        .device = &Device,
    };
    dtConfig *list[] = {&cfg1};

    volatile uint8_t sensor_counter = 0;
    dtHA_States prevState = -1;
    uint8_t status_counter = 0;
    while(1)
    {
        start = xTaskGetTickCount();
        if(prevState != state)
        {
            printf("HA_Runnable - state: %i -> %i, tick: %lu\n", prevState, state, start);
            prevState = state;
        }
        switch(state)
        {
            case HA_InitMqtt:
                mqtt_init(broker_url);
                state = HA_Idle;
                ESP_LOGI(TAG, "MQTT has been initialized");
                break;
            case HA_Idle:
                if(enable_flag != 0)
                {
                    state = HA_StartMqtt;
                }
                break;
            case HA_StartMqtt:
                mqtt_app_start();
                state = HA_WaitMqtt; 
                ESP_LOGI(TAG, "MQTT has started");
                status_counter = 0;
                break;
            case HA_WaitMqtt:
                if(MQTT_GetState() == MQTT_State_Connected)
                {
                    state = HA_SendDiscovery;
                    ESP_LOGI(TAG, "MQTT has been connected");
                    sensor_counter = 0;
                }
                else if(status_counter >= 100)
                {
                    mqtt_reconnect();
                    status_counter = 0;
                }
                status_counter ++;
                break;
            case HA_SendDiscovery:
                if(sensor_counter < (sizeof(list)/sizeof(list[0])))
                {
                    char topic[64];
                    char msg[360];
                    CreateSensor(list[sensor_counter],  topic, msg);
                    if(mqtt_publish(topic, msg, 0, 0, 0) == 0)
                    {
                        if(list[sensor_counter]->cmd_topic != 0)
                        {
                            mqtt_subscribe((uint8_t*)(list[sensor_counter]->cmd_topic));
                        }
                        if(list[sensor_counter]->default_state != 0)
                        {
                            mqtt_publish(list[sensor_counter]->state_topic, list[sensor_counter]->default_state, 0, 0, 0);
                        }
                        list[sensor_counter]->period_counter = list[sensor_counter]->handle_period;
                        sensor_counter++;
                        ESP_LOGI(TAG, "MQTT discovery message has been sent: %i", sensor_counter);
                        //ESP_LOGI(TAG, "%s -> %s", msg, topic);
                    }
                }
                else
                {
                    state = HA_Running;
                }
                
                break;
            case HA_Running:
            if((enable_flag != 0))
            {
                if((MQTT_GetState() == MQTT_State_Connected))
                {
                    uint8_t i = 0;
                    uint8_t data[16];
                    while(i < (sizeof(list)/sizeof(list[0])))
                    {
                        if(list[i]->cmd_topic != nullptr)
                        {
                            if(mqtt_get_data((uint8_t*)list[i]->cmd_topic, (uint8_t*)data) == 0)
                            {
                                if(list[i]->cmd_callback != nullptr)
                                {
                                    list[i]->cmd_callback(data);
                                }
                            }
                        }

                        if(list[i]->state_callback != nullptr)
                        {
                            if(list[i]->period_counter >= list[i]->handle_period)
                            {
                                if(list[i]->state_callback(data) != 0)
                                {
                                    printf("ha - state_callback data: %s\n", data);
                                    if(list[i]->state_topic != nullptr)
                                    {
                                        if(mqtt_publish(list[i]->state_topic, (char*)data, 0, 0, 0) == 0)
                                        {
                                            list[i]->period_counter = 0;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                list[i]->period_counter++;
                            }
                        }
                        i++;
                    }
                }
                else
                {
                    state = HA_Idle;
                }
            }
            else
            {
                state = HA_Stopping;
            }
                break;
            case HA_Stopping:
                mqtt_stop();
                state = HA_Idle;
                break;
        }
        end = xTaskGetTickCount();
        if((100 / portTICK_PERIOD_MS) > (end - start))
        {
            vTaskDelay((100 / portTICK_PERIOD_MS) - ((end - start)));
        }
    }
}

uint8_t HA_IsIdle(void)
{
    return (state == HA_Idle) && (enable_flag == 0);
}

void HA_Enable(uint8_t enable)
{
    enable_flag = enable;
}
