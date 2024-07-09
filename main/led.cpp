#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"

#include "led.h"
#include "hw_conf.h"


#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_RESOLUTION LEDC_TIMER_8_BIT

#define BTN_FULL_BRIGHT 0x3C
#define BTN_HALF_BRIGHT 0x28
#define BTN_ZERO_BRIGHT 0x00

#define BTN_POWER_BACKLIGHT 0x6A
#define BTN_BRIGHTNESS_BACKLIGHT 0x6C
#define BTN_MODE_BACKLIGHT 0x6E


#define TAG "LED"


static uint8_t rgb_brightness;


void led_rgb_init() {
    esp_err_t err;

    // Prepare and configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER_RGB,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_conf_r = {
        .gpio_num = GPIO_RGB_R,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_RGB_R,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_RGB,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ledc_conf_r);

    ledc_channel_config_t ledc_conf_g = {
        .gpio_num = GPIO_RGB_G,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_RGB_G,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_RGB,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ledc_conf_g);

    ledc_channel_config_t ledc_conf_b = {
        .gpio_num = GPIO_RGB_B,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_RGB_B,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_RGB,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ledc_conf_b);

    // Initialize fade service.
    ledc_fade_func_install(0);
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


void cms_send(uint8_t command, uint8_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, command, 1);
    i2c_master_write_byte(cmd, value, 1);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, -1));
}

// brightness from 0 (off) to 8 (max)
void led_status_brightness(uint8_t brightness) {
    uint8_t cmd;
    if (brightness == 0) {
        cmd = 0x00;
    } else {
        if (brightness > 8) {
            brightness = 8;
        }
        cmd = brightness << 4 | 0x01;
    }
    cms_send(0x48, cmd);
}

// level 0 (off), 1 (only power button), 2 (everything but dim), 3 (everything max brightness)
void led_set_brightness(uint8_t level) {
    if (level == 0) {
        led_status_brightness(0);
        led_rgb_set(0,0,0);
        rgb_brightness = 0;
        
    } else if (level == 1) {
        led_status_brightness(1);
        cms_send(BTN_POWER_BACKLIGHT, BTN_HALF_BRIGHT);
        cms_send(BTN_BRIGHTNESS_BACKLIGHT, BTN_ZERO_BRIGHT);
        cms_send(BTN_MODE_BACKLIGHT, BTN_ZERO_BRIGHT);
        led_rgb_set(0,0,0);
        rgb_brightness = 0;

    } else if (level == 2) {
        led_status_brightness(1);
        cms_send(BTN_POWER_BACKLIGHT, BTN_FULL_BRIGHT);
        cms_send(BTN_BRIGHTNESS_BACKLIGHT, BTN_FULL_BRIGHT);
        cms_send(BTN_MODE_BACKLIGHT, BTN_FULL_BRIGHT);
        // led_rgb_set(0,0,0);
        rgb_brightness = 64;

    } else if (level == 3) {
        led_status_brightness(1);
        cms_send(BTN_POWER_BACKLIGHT, BTN_FULL_BRIGHT);
        cms_send(BTN_BRIGHTNESS_BACKLIGHT, BTN_FULL_BRIGHT);
        cms_send(BTN_MODE_BACKLIGHT, BTN_FULL_BRIGHT);
        // led_rgb_set(0,0,0);
        rgb_brightness = 255;
    }
}

void led_status_set_indicators(uint8_t indicators) {
    cms_send(0x68, indicators);
}

void led_status_init() {
    // Configure I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_LED_SDA,
        .scl_io_num = GPIO_LED_SCL,
        .master = { .clk_speed = 100000 },
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

// Brightness 0 to 8, 0 is off


void led_init() {
    led_rgb_init();
    led_status_init();


    led_set_brightness(3);
    led_status_set_indicators(LED_IND_WIFI);
    led_rgb_set(0,0,255);
}