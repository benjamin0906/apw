#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define TURN_ON_DURATION 45

void WC_Runnable(void);
void WC_CmdCallback(uint8_t *cmd);

static uint8_t command;
static uint8_t status;

void WC_CmdCallback(uint8_t *cmd)
{
    if(cmd != nullptr)
    {
        command = cmd[0];
        printf("watering_control - %s\n", cmd);
    }
}

uint8_t WC_StateCallback(uint8_t *state)
{
    static uint8_t previous_status = 0;
    uint8_t ret = 0;
    if((state != nullptr) && ((previous_status != status)))
    {
        previous_status = status;
        *(state++) = status;
        *state = 0;
        ret = 1;
    }
    return ret;
}

void WC_Runnable(void)
{
    uint32_t start_tick = 0;
    uint32_t end_tick = 0;
    gpio_num_t driver_pins[] = {GPIO_NUM_0, GPIO_NUM_1, GPIO_NUM_4, GPIO_NUM_3, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_20};
    uint32_t start_time = 0;
    uint8_t on_flag = 0;
    {
        uint8_t i = 0;
        gpio_config_t pin_cfg = {
        .intr_type = GPIO_INTR_DISABLE, 
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 0,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE};
        for(i = 0; i < (sizeof(driver_pins)/sizeof(driver_pins[0])); i++)
        {
            pin_cfg.pin_bit_mask = (1ULL << driver_pins[i]);
            gpio_config(&pin_cfg);
            gpio_sleep_sel_dis(driver_pins[i]);
            gpio_set_drive_capability(driver_pins[i], GPIO_DRIVE_CAP_3);
            gpio_set_level(driver_pins[i], 0);
        }
    }
    
    while(1)
    {
        start_tick = xTaskGetTickCount();

        if(on_flag != 0)
        {
            uint32_t ellapsed_time = (esp_timer_get_time() / 1000000) - start_time;

            if(ellapsed_time  >= TURN_ON_DURATION)
            {
                command = '0';
            }
        }

        if((status != command) && (command >= '0') && (command <= '8'))
        {
            uint8_t index = command - '0';

            /* Turning off all the pumps */
            for(int i = 0; i < (sizeof(driver_pins)/sizeof(driver_pins[0])); i++)
            {
                gpio_set_level(driver_pins[i], 0);
            }
            on_flag = 0;

            /* If a pump is selected turn it on */
            if(index != 0)
            {
                index--;
                on_flag = 1;
                start_time = esp_timer_get_time() / 1000000;
                gpio_set_level(driver_pins[index], 1);
                printf("Turned on: %i\n", driver_pins[index]);
            }

            status = command;
        }

        end_tick = xTaskGetTickCount();
        if((100 / portTICK_PERIOD_MS) > (end_tick - start_tick))
        {
            vTaskDelay((100 / portTICK_PERIOD_MS) - ((end_tick - start_tick)));
        }
    }
    
    
}