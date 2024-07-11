#pragma once

#include "hw_conf.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"


#define BUTTON_POWER (static_cast<uint8_t>(GPIO_BTN_POWER))
#define BUTTON_BRIGHTNESS (static_cast<uint8_t>(GPIO_BTN_BRIGHTNESS))
#define BUTTON_MODE (static_cast<uint8_t>(GPIO_BTN_MODE))


extern QueueHandle_t button_queue;

// Structure to represent a button event
typedef struct {
    uint8_t pin;      // Button pin identifier
    bool longPress;   // True if long press, false if short press
} ButtonEvent;

// Function prototypes
void buttons_init();
void button_task(void *pvParameters);

