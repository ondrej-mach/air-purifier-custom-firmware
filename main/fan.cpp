#include "hw_conf.h"
#include "fan.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <math.h>

static uint8_t fan_current_percentage;

#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_RESOLUTION LEDC_TIMER_8_BIT


void fan_init() {
    // Prepare and configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER_MOTOR_PWM,
        .freq_hz = 500,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    // Prepare and configure the LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num   = GPIO_MOTOR_PWM,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL_MOTOR_PWM,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_MOTOR_PWM,
        .duty       = 0,
        .flags      = { .output_invert = 1 },
    };
    ledc_channel_config(&ledc_channel);

    // Set up 5V supply control pin (active HIGH)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_MOTOR_5V),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_MOTOR_5V, 0);

    // Break pin (active LOW)
    gpio_set_direction(GPIO_MOTOR_BRK, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_MOTOR_BRK, 0);
}

uint8_t fan_get_percentage() {
    return fan_current_percentage;
}

void fan_set_percentage(uint8_t percentage) {
    // Deactivate break after driver is ready
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
        duty = 128;
        freq = 100 + ((percentage-1) * (510 - 99) / 99);
    }

    // Deactivate break
    gpio_set_level(GPIO_MOTOR_BRK, 1);
    gpio_set_level(GPIO_MOTOR_5V, supply_voltage);
    ledc_set_freq(LEDC_MODE, LEDC_TIMER_MOTOR_PWM, freq);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_MOTOR_PWM, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_MOTOR_PWM);
}

void fan_set_power(bool val) {
    
}