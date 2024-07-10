#include "hw_conf.h"
#include "buttons.h"

#include "driver/gpio.h"
#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static TaskHandle_t notification_handler = NULL;


// GPIO interrupt handler
void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint8_t button = (uint8_t)(uintptr_t)arg;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Notify the task that the interrupt occurred and pass the value
    xTaskNotifyFromISR(notification_handler, button, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    // If xHigherPriorityTaskWoken was set to true, a context switch should be performed
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}



// Button initialization function
void buttons_init(TaskHandle_t task) {
    notification_handler = task;

    // Configure buttons as input
    gpio_set_direction(GPIO_BTN_POWER, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_BTN_BRIGHTNESS, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_BTN_MODE, GPIO_MODE_INPUT);

    // Configure GPIO interrupt
    gpio_set_intr_type(GPIO_BTN_POWER, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(GPIO_BTN_BRIGHTNESS, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(GPIO_BTN_MODE, GPIO_INTR_NEGEDGE);

    // Install ISR service
    gpio_install_isr_service(0); // TODO any flags needed?

    // Attach interrupt handlers
    gpio_isr_handler_add(GPIO_BTN_POWER, gpio_isr_handler, (void*) BUTTON_POWER);
    gpio_isr_handler_add(GPIO_BTN_BRIGHTNESS, gpio_isr_handler, (void*) BUTTON_BRIGHTNESS);
    gpio_isr_handler_add(GPIO_BTN_MODE, gpio_isr_handler, (void*) BUTTON_MODE);
}