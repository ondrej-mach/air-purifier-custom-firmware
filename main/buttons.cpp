#include "hw_conf.h"
#include "buttons.h"

#include "driver/gpio.h"
#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"


#define QUEUE_LENGTH 4
#define QUEUE_ITEM_SIZE sizeof(ButtonEvent)
#define LONG_PRESS_TIME_MS 7000


QueueHandle_t button_queue;


// Timer handles for detecting long presses
static TimerHandle_t powerButtonTimer;
static TimerHandle_t brightnessButtonTimer;
static TimerHandle_t modeButtonTimer;


static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint8_t pin = (uint8_t)(uintptr_t)arg;
    // Active LOW
    bool pressed = !gpio_get_level(static_cast<gpio_num_t>(pin));

    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (pressed) {
        ButtonEvent event = { .pin = pin, .longPress = false };
        xQueueSendFromISR(button_queue, &event, &xHigherPriorityTaskWoken);

        // Start the appropriate timer to detect a long press
        switch (pin) {
            case BUTTON_POWER:
                xTimerStartFromISR(powerButtonTimer, &xHigherPriorityTaskWoken);
                break;
            case BUTTON_BRIGHTNESS:
                xTimerStartFromISR(brightnessButtonTimer, &xHigherPriorityTaskWoken);
                break;
            case BUTTON_MODE:
                xTimerStartFromISR(modeButtonTimer, &xHigherPriorityTaskWoken);
                break;
            default:
                break;
        }
    } else {
        // Stop the timer as soon as the button is released
        switch (pin) {
            case BUTTON_POWER:
                xTimerStopFromISR(powerButtonTimer, &xHigherPriorityTaskWoken);
                break;
            case BUTTON_BRIGHTNESS:
                xTimerStopFromISR(brightnessButtonTimer, &xHigherPriorityTaskWoken);
                break;
            case BUTTON_MODE:
                xTimerStopFromISR(modeButtonTimer, &xHigherPriorityTaskWoken);
                break;
            default:
                break;
        }
    }

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void long_press_timer_callback(TimerHandle_t xTimer) {
    uint8_t pin = (uint8_t)(uintptr_t)pvTimerGetTimerID(xTimer);
    ButtonEvent event = { .pin = pin, .longPress = true };

    // Send the long press event to the queue
    xQueueSend(button_queue, &event, portMAX_DELAY);
}

void buttons_init() {
    button_queue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);

    if (button_queue == NULL) {
        // Handle error: Queue could not be created
        return;
    }

    // Configure buttons as input
    gpio_set_direction(GPIO_BTN_POWER, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_BTN_BRIGHTNESS, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_BTN_MODE, GPIO_MODE_INPUT);

    // Configure GPIO interrupt
    gpio_set_intr_type(GPIO_BTN_POWER, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(GPIO_BTN_BRIGHTNESS, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(GPIO_BTN_MODE, GPIO_INTR_ANYEDGE);

    // Install ISR service
    gpio_install_isr_service(0);

    // Attach interrupt handlers
    gpio_isr_handler_add(GPIO_BTN_POWER, gpio_isr_handler, (void*) BUTTON_POWER);
    gpio_isr_handler_add(GPIO_BTN_BRIGHTNESS, gpio_isr_handler, (void*) BUTTON_BRIGHTNESS);
    gpio_isr_handler_add(GPIO_BTN_MODE, gpio_isr_handler, (void*) BUTTON_MODE);

    // Create timers for long press detection
    powerButtonTimer = xTimerCreate("PowerButtonTimer", pdMS_TO_TICKS(LONG_PRESS_TIME_MS), pdFALSE, (void*) BUTTON_POWER, long_press_timer_callback);
    brightnessButtonTimer = xTimerCreate("BrightnessButtonTimer", pdMS_TO_TICKS(LONG_PRESS_TIME_MS), pdFALSE, (void*) BUTTON_BRIGHTNESS, long_press_timer_callback);
    modeButtonTimer = xTimerCreate("ModeButtonTimer", pdMS_TO_TICKS(LONG_PRESS_TIME_MS), pdFALSE, (void*) BUTTON_MODE, long_press_timer_callback);
}