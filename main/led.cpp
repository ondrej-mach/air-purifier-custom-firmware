#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "led.h"
#include "hw_conf.h"


#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_RESOLUTION LEDC_TIMER_8_BIT

#define TAG "LED"


void led_rgb_init() {
    esp_err_t err;

    ledc_timer_config_t timer_conf;
    timer_conf.speed_mode = LEDC_MODE;
    timer_conf.timer_num = LEDC_TIMER_RGB;
    timer_conf.freq_hz = 1000;
    timer_conf.duty_resolution = LEDC_RESOLUTION;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ledc_conf_r = {
        .gpio_num = GPIO_RGB_R,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_RGB_R,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_RGB,
        .duty = 0,
        .hpoint = 0,
    };
    err = ledc_channel_config(&ledc_conf_r);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel R configuration failed: %s", esp_err_to_name(err));
    }

    ledc_channel_config_t ledc_conf_g = {
        .gpio_num = GPIO_RGB_G,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_RGB_G,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_RGB,
        .duty = 0,
        .hpoint = 0,
    };
    err = ledc_channel_config(&ledc_conf_g);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel G configuration failed: %s", esp_err_to_name(err));
    }

    ledc_channel_config_t ledc_conf_b = {
        .gpio_num = GPIO_RGB_B,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_RGB_B,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_RGB,
        .duty = 0,
        .hpoint = 0,
    };
    err = ledc_channel_config(&ledc_conf_b);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel B configuration failed: %s", esp_err_to_name(err));
    }

    // Initialize fade service.
    err = ledc_fade_func_install(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC fade function installation failed: %s", esp_err_to_name(err));
    }
}


void led_rgb_set(uint32_t red, uint32_t green, uint32_t blue) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RGB_R, red);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RGB_G, green);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_RGB_B, blue);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RGB_R);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RGB_G);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_RGB_B);
}


void led_rgb_fade(uint32_t red, uint32_t green, uint32_t blue, int fade_time_ms) {
    ledc_set_fade_time_and_start(LEDC_MODE, LEDC_CHANNEL_RGB_R, red, fade_time_ms, LEDC_FADE_NO_WAIT);
    ledc_set_fade_time_and_start(LEDC_MODE, LEDC_CHANNEL_RGB_G, green, fade_time_ms, LEDC_FADE_NO_WAIT);
    ledc_set_fade_time_and_start(LEDC_MODE, LEDC_CHANNEL_RGB_B, blue, fade_time_ms, LEDC_FADE_NO_WAIT);
}


void led_status_init() {

}


void led_init() {
    led_rgb_init();
    led_status_init();
}