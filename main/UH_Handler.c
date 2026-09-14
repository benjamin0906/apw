    pin_led.pin_bit_mask = 1ULL << GPIO_NUM_0;
    gpio_config(&pin_led);
    pin_led.pin_bit_mask = 1ULL << GPIO_NUM_1;
    pin_led.mode = GPIO_MODE_INPUT;
    gpio_config(&pin_led);

    uint32_t timestamp;
    uint32_t measured_time = 0;

gpio_set_level(GPIO_NUM_0, 1);
            timestamp = esp_timer_get_time();
            while(((uint32_t)esp_timer_get_time() - timestamp) <= 10)
            {
            }
            gpio_set_level(GPIO_NUM_0, 0);
            while((gpio_get_level(GPIO_NUM_1) == 0) && (((uint32_t)esp_timer_get_time() - timestamp) <= 1000000));
            timestamp = esp_timer_get_time();
            while((gpio_get_level(GPIO_NUM_1) == 1)  && (((uint32_t)esp_timer_get_time() - timestamp) <= 1000000));
            measured_time = ((uint32_t)esp_timer_get_time() - timestamp);

            printf("Measured time in us: %lu\n", measured_time);