#pragma once

#define GPIO_RGB_R GPIO_NUM_21
#define GPIO_RGB_G GPIO_NUM_32
#define GPIO_RGB_B GPIO_NUM_33

#define GPIO_BUZZER GPIO_NUM_4

#define GPIO_BTN_POWER GPIO_NUM_19
#define GPIO_BTN_BRIGHTNESS GPIO_NUM_18
#define GPIO_BTN_MODE GPIO_NUM_5

#define GPIO_LED_SDA GPIO_NUM_14
#define GPIO_LED_SCL GPIO_NUM_27

#define GPIO_EEPROM_SDA GPIO_NUM_36
#define GPIO_EEPROM_SCL GPIO_NUM_37

#define GPIO_MOTOR_PWM GPIO_NUM_26 // OUT
#define GPIO_MOTOR_BRK GPIO_NUM_2  // OUT
#define GPIO_MOTOR_FG GPIO_NUM_34  // IN
#define GPIO_MOTOR_5V GPIO_NUM_15  // OUT

// PS for particle sensor
#define GPIO_PS_RX GPIO_NUM_16
#define GPIO_PS_TX GPIO_NUM_17
#define GPIO_PS_5V GPIO_NUM_13


// ESP32 peripherals

#define LEDC_TIMER_MOTOR_PWM LEDC_TIMER_0
#define LEDC_TIMER_RGB LEDC_TIMER_1
#define LEDC_TIMER_BUZZER LEDC_TIMER_2

#define LEDC_CHANNEL_MOTOR_PWM LEDC_CHANNEL_0
#define LEDC_CHANNEL_RGB_R LEDC_CHANNEL_1
#define LEDC_CHANNEL_RGB_G LEDC_CHANNEL_2
#define LEDC_CHANNEL_RGB_B LEDC_CHANNEL_3
#define LEDC_CHANNEL_BUZZER LEDC_CHANNEL_4

#define BUZZER_FREQUENCY 2000
#define BUZZER_BEEP_TIME_MS 60

