#pragma once

#include <cstdint>
#include "hw_conf.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_POWER (static_cast<uint8_t>(GPIO_BTN_POWER))
#define BUTTON_BRIGHTNESS (static_cast<uint8_t>(GPIO_BTN_BRIGHTNESS))
#define BUTTON_MODE (static_cast<uint8_t>(GPIO_BTN_MODE))


void buttons_init(TaskHandle_t task);
