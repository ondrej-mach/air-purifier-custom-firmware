#include "hw_conf.h"
#include "fan.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <math.h>

static uint8_t fan_current_percentage;

void fan_init() {
    // Prepare and configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 500,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    // Prepare and configure the LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num   = GPIO_MOTOR_PWM,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
    };
    ledc_channel_config(&ledc_channel);

    // Set up 5V supply control pin
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_MOTOR_5V);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Put the fan in a defined state
    fan_set_percentage(0);

    // Deactivate break after driver is ready
    io_conf.pin_bit_mask = (1ULL << GPIO_MOTOR_BRK);
    gpio_config(&io_conf);
    gpio_set_level(static_cast<gpio_num_t>(GPIO_MOTOR_BRK), 1);
}

uint8_t fan_get_percentage() {
    return fan_current_percentage;
}

void fan_set_percentage(uint8_t percentage) {
    if (percentage > 100) {
        percentage = 100;
    }

    bool supply_voltage;
    uint32_t duty;
    uint32_t freq;

    if (percentage == 0) {
        supply_voltage = false;
        duty = 0;
        freq = 100;
    } else {
        supply_voltage = true;
        duty = 512;
        freq = 100 + ((percentage-1) * (510 - 99) / 99);
    }

    gpio_set_level(static_cast<gpio_num_t>(GPIO_MOTOR_5V), supply_voltage);
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void fan_set_power(bool val) {
    
}