#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Structure to hold the PM2.5 value and air quality enum
struct aq_queue_item_t {
    int pm25;
    int air_quality_enum;
};


void pms_init(QueueHandle_t queue);
