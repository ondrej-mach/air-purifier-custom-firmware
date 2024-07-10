#include "buzzer.h"
#include "hw_conf.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"


#define BUZZER_DUTY_RES LEDC_TIMER_8_BIT
#define BUZZER_DUTY (1 << (BUZZER_DUTY_RES - 1))


static void buzzer_beep_task(void *arg);

void buzzer_init() {
    // Configure the LEDC timer
    ledc_timer_config_t buzzer_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = BUZZER_DUTY_RES,
        .timer_num        = LEDC_TIMER_BUZZER,
        .freq_hz          = BUZZER_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&buzzer_timer);

    // Configure the LEDC channel
    ledc_channel_config_t buzzer_channel = {
        .gpio_num       = GPIO_BUZZER,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_BUZZER,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_BUZZER,
        .duty           = 0, // Initially off
        .hpoint         = 0
    };
    ledc_channel_config(&buzzer_channel);
}

void buzzer_beep() {
    xTaskCreate(buzzer_beep_task, "buzzer_beep_task", 2048, NULL, 10, NULL);
}

static void buzzer_beep_task(void *arg) {
    // Turn on the buzzer
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_BUZZER, BUZZER_DUTY);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_BUZZER);

    // Wait for the beep duration
    vTaskDelay(pdMS_TO_TICKS(BUZZER_BEEP_TIME_MS));

    // Turn off the buzzer
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_BUZZER, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_BUZZER);

    // Delete the task as it's no longer needed
    vTaskDelete(NULL);
}