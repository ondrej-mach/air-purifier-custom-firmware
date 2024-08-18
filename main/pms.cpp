#include "pms.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "led.h"
#include "hw_conf.h"

#include <esp_matter_cluster.h>

using namespace chip::app::Clusters::AirQuality;

#define PMS_BAUD_RATE  9600
#define BUF_SIZE        1024

// Command to request PM2.5 value
static const char PMS_CMD[] = {0x11, 0x02, 0x0b, 0x01, 0xe1};

static QueueHandle_t air_quality_queue;

// According to Chinese IAQI standard (also used by Xiaomi in original firmware)
static int pm25_to_aq_enum(int pm25) {
    if (pm25 < 35) {
        return static_cast<int>(AirQualityEnum::kGood);
    }
    if (pm25 < 75) {
        return static_cast<int>(AirQualityEnum::kFair);
    }
    if (pm25 < 115) {
        return static_cast<int>(AirQualityEnum::kModerate);
    }
    if (pm25 < 150) {
        return static_cast<int>(AirQualityEnum::kPoor);
    }
    if (pm25 <= 500) {
        return static_cast<int>(AirQualityEnum::kVeryPoor);
    }
    
    return static_cast<int>(AirQualityEnum::kExtremelyPoor);
}

static bool validate_checksum(const uint8_t *data, int len) {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum == 0;
}

// Task to communicate with the PMS sensor
static void pms_task(void *pvParameters) {
    static uint8_t uart_recv_buffer[BUF_SIZE];
    static aq_queue_item_t aq_queue_item;
    

    while (1) {
        // Send command to PMS sensor
        uart_write_bytes(UART_PMS, PMS_CMD, sizeof(PMS_CMD));

        // Read response from PMS sensor
        int len = uart_read_bytes(UART_PMS, uart_recv_buffer, BUF_SIZE, pdMS_TO_TICKS(100));

        // Validate response and parse PM2.5 value
        bool valid = len >= 20;
        valid &= uart_recv_buffer[0] == 0x16;
        valid &= uart_recv_buffer[1] == 0x11;
        valid &= uart_recv_buffer[2] == 0x0b;
        valid &= validate_checksum(uart_recv_buffer, 20);

        if (valid) {
            int pm25_value = (uart_recv_buffer[15] << 8) | uart_recv_buffer[16];
            aq_queue_item.pm25 = pm25_value;
            aq_queue_item.air_quality_enum = pm25_to_aq_enum(pm25_value);
        } else {
            aq_queue_item.air_quality_enum = static_cast<int>(AirQualityEnum::kUnknown);
        }

        // Send the data to the queue
        xQueueSend(air_quality_queue, &aq_queue_item, portMAX_DELAY);

        // Delay for 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// PMS initialization function
void pms_init(QueueHandle_t queue) {
    // Initialize GPIO for PMS sensor power
    gpio_set_direction(GPIO_PMS_5V, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_MOTOR_BRK, 0);

    const uart_config_t uart_config = {
        .baud_rate = PMS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PMS, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PMS, GPIO_PMS_TX, GPIO_PMS_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PMS, BUF_SIZE, 0, 0, NULL, 0));

    // Save the queue handle
    air_quality_queue = queue;

    // Create the PMS task
    xTaskCreate(pms_task, "pms_task", 2048, NULL, 5, NULL);
}
