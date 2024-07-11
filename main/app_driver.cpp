/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <led_driver.h>
#include <app_driver.h>
#include <fan.h>
#include <led.h>
#include "buttons.h"
#include "buzzer.h"

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

#include <device.h>
#include <esp_matter.h>

#include <esp_wifi.h>
#include <esp_event.h>


using namespace chip::app::Clusters;
using namespace chip::app;
using namespace esp_matter;


static const char *TAG = "app_driver";
extern uint16_t air_purifier_endpoint_id;
extern uint16_t air_quality_sensor_endpoint_id;

extern bool device_commisioning;

// Things that are not handled by matter database
struct State {
    // legal values are 1, 2, 3
    uint8_t brightness = 3;
    // stores the previous state while the fan is off
    FanControl::FanModeEnum prev_mode = FanControl::FanModeEnum::kHigh;
    // Stores the previous percentage setting while the fan is off
    // When turning on, percentage takes precedence over mode (except when 0)
    uint8_t prev_percentage = 0;
};


static State state;


void app_driver_update_fan_speed(uint8_t percentage);

void app_driver_report_fan_mode_from_percentage(uint8_t percentage);



void app_driver_update_fan_speed(uint8_t percentage) {
    // Hardware
    fan_set_percentage(percentage);

    // HW + matter DB (do not want slow display response)
    app_driver_report_fan_mode_from_percentage(percentage);

    // Matter DB
    using namespace FanControl::Attributes;
    uint16_t endpoint_id = air_purifier_endpoint_id;
    uint32_t cluster_id = FanControl::Id;
    
    esp_matter_attr_val_t val;
    val = esp_matter_nullable_uint8(percentage);
    attribute::report(endpoint_id, cluster_id, PercentSetting::Id, &val);
    attribute::report(endpoint_id, cluster_id, SpeedSetting::Id, &val);
    val = esp_matter_uint8(percentage);
    attribute::report(endpoint_id, cluster_id, PercentCurrent::Id, &val);
    attribute::report(endpoint_id, cluster_id, SpeedCurrent::Id, &val);

    // State outside of matter db
    if (percentage != 0) {
        state.prev_percentage = percentage;
    }
}


void app_driver_show_mode(FanControl::FanModeEnum mode) {
    if (mode == FanControl::FanModeEnum::kOff) {
        led_set_brightness(0);
    } 
    else {
        led_set_brightness(state.brightness);
        led_status_set_off(LED_IND_HEART | LED_IND_NIGHT | LED_IND_AUTO);
        if (mode == FanControl::FanModeEnum::kHigh) {
            led_status_set_on(LED_IND_HEART);
        } else if (mode == FanControl::FanModeEnum::kLow) {
            led_status_set_on(LED_IND_NIGHT);
        } else if (mode == FanControl::FanModeEnum::kAuto) {
            led_status_set_on(LED_IND_AUTO);
        }
    }
}


void app_driver_report_fan_mode_from_percentage(uint8_t percentage) {
    using namespace FanControl::Attributes;

    uint16_t endpoint_id = air_purifier_endpoint_id;
    uint32_t cluster_id = FanControl::Id;

    esp_matter_attr_val_t val;
    FanControl::FanModeEnum mode;
    
    if (percentage == 0) {
        mode = FanControl::FanModeEnum::kOff;
    } else if (percentage <= 30) {
        mode = FanControl::FanModeEnum::kLow;
    } else {
        mode = FanControl::FanModeEnum::kHigh;
    }
    
    // HW
    app_driver_show_mode(mode);

    // Matter DB
    val = esp_matter_enum8(static_cast<uint8_t>(mode));
    attribute::report(endpoint_id, cluster_id, FanMode::Id, &val);
}



void app_driver_update_mode(uint8_t mode)
{
    using namespace FanControl::Attributes;

    uint16_t endpoint_id = air_purifier_endpoint_id;
    uint32_t cluster_id = FanControl::Id;

    FanControl::FanModeEnum m = static_cast<FanControl::FanModeEnum>(mode);

    uint8_t percentage = 0;

    switch (m) {
        case FanControl::FanModeEnum::kOff:
            percentage = 0;
            break;
        case FanControl::FanModeEnum::kLow:
            percentage = 20;
            break;
        case FanControl::FanModeEnum::kHigh:
            percentage = 100;
            break;
        case FanControl::FanModeEnum::kAuto:
            percentage = 42;
            break;
        default:
            break;
    }
    
    if (m == FanControl::FanModeEnum::kAuto) {
        // Set hardware
        fan_set_percentage(percentage);
        app_driver_show_mode(m);

        // save to matter DB  
        esp_matter_attr_val_t val = esp_matter_uint8(percentage);
        attribute::report(endpoint_id, cluster_id, PercentCurrent::Id, &val);
        attribute::report(endpoint_id, cluster_id, SpeedCurrent::Id, &val);

        // save to state
        state.prev_mode = FanControl::FanModeEnum::kAuto;
        state.prev_percentage = 0;
    } else {
        app_driver_update_fan_speed(percentage);
    }
}




void app_driver_update_air_quality()
{
    uint16_t endpoint_id = air_quality_sensor_endpoint_id;
    uint32_t cluster_id = AirQuality::Id;
    uint32_t attribute_id = AirQuality::Attributes::AirQuality::Id;

    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = cluster::get(endpoint, cluster_id);
    attribute_t *attribute = attribute::get(cluster, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    val.val.u8 = static_cast<uint8_t>(AirQuality::AirQualityEnum::kFair);

    attribute::report(endpoint_id, cluster_id, attribute_id, &val);
    /*
    val = esp_matter_null15;
    val.type = ESP_MATTER_VAL_TYPE_UINT32;
    attribute::report(
        endpoint_id,
        Pm25ConcentrationMeasurement::Id,
        Pm25ConcentrationMeasurement::Attributes::LevelValue::Id,
        &val
    );

    attribute::report(
        endpoint_id,
        Pm25ConcentrationMeasurement::Id,
        Pm25ConcentrationMeasurement::Attributes::MeasurementUnit,
        &val
    );
    */
}



esp_err_t app_driver_air_purifier_set_defaults(uint16_t endpoint_id)
{
    
    esp_err_t err = ESP_OK;
    /*  Not really important
    void *priv_data = endpoint::get_priv_data(endpoint_id);
    led_driver_handle_t handle = (led_driver_handle_t)priv_data;
    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = NULL;
    attribute_t *attribute = NULL;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    // Setting power
    cluster = cluster::get(endpoint, OnOff::Id);
    attribute = attribute::get(cluster, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    err |= app_driver_room_air_conditioner_set_power(handle, &val);

*/
    return err;
    
}

void app_driver_buttons_callback(uint8_t button) {
    using namespace FanControl::Attributes;

    uint16_t endpoint_id = air_purifier_endpoint_id;
    uint32_t cluster_id = FanControl::Id;
    uint32_t mode_attr_id = FanControl::Attributes::FanMode::Id;
    uint32_t speed_attr_id = FanControl::Attributes::SpeedSetting::Id;

    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = cluster::get(endpoint, cluster_id);
    attribute_t *attribute = attribute::get(cluster, mode_attr_id);

    esp_matter_attr_val_t val;
    attribute::get_val(attribute, &val);
    FanControl::FanModeEnum mode = static_cast<FanControl::FanModeEnum>(val.val.u8);

    if (mode == FanControl::FanModeEnum::kOff) {
        if (button == BUTTON_POWER) {
            buzzer_beep();
            if (state.prev_percentage == 0) {
                val = esp_matter_enum8(static_cast<uint8_t>(state.prev_mode));
                attribute::update(endpoint_id, cluster_id, mode_attr_id, &val);
            } else {
                val = esp_matter_uint8(state.prev_percentage);
                attribute::update(endpoint_id, cluster_id, speed_attr_id, &val);
            }
        }
    } else {
        buzzer_beep();
        if (state.brightness <= 1) {
            state.brightness = 3;
            led_set_brightness(state.brightness);
        
        } else {
            if (button == BUTTON_POWER) {
                val = esp_matter_enum8(static_cast<uint8_t>(FanControl::FanModeEnum::kOff));
                attribute::update(endpoint_id, cluster_id, mode_attr_id, &val);

            } else if (button == BUTTON_BRIGHTNESS) {
                // Decrement (2 or 3), 1 is caught earlier
                state.brightness--;
                led_set_brightness(state.brightness);

            } else if (button == BUTTON_MODE) {
                // High -> Low -> Auto
                FanControl::FanModeEnum new_mode;
                if (mode == FanControl::FanModeEnum::kHigh) {
                    new_mode = FanControl::FanModeEnum::kLow;
                } else if (mode == FanControl::FanModeEnum::kLow) {
                    new_mode = FanControl::FanModeEnum::kAuto;
                } else {
                    new_mode = FanControl::FanModeEnum::kHigh;
                }
                val = esp_matter_enum8(static_cast<uint8_t>(new_mode));
                attribute::update(endpoint_id, cluster_id, mode_attr_id, &val);
            }
        }
    }
}


void wireless_monitor_task(void *pvParameters) {
    while (1) {
        // Check Wi-Fi connection status
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            led_status_set_on(LED_IND_WIFI);
        } else {
            if (device_commisioning) {
                led_status_set_blink(LED_IND_WIFI);
            } else {
                led_status_set_off(LED_IND_WIFI);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void app_driver_hw_init() {
    fan_init();
    led_init();
    buzzer_init();
    buttons_init();
    xTaskCreate(&wireless_monitor_task, "wireless_monitor", 4096, NULL, 5, NULL);

    led_set_brightness(0);
    led_rgb_set(0,0,255);
}



esp_err_t app_driver_attribute_update(uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;

    if (endpoint_id == air_purifier_endpoint_id) {
        if (cluster_id == FanControl::Id) {
            if (attribute_id == FanControl::Attributes::FanMode::Id) {
                app_driver_update_mode(val->val.u8);

            } else if (attribute_id == FanControl::Attributes::PercentSetting::Id
                || attribute_id == FanControl::Attributes::SpeedSetting::Id) {

                uint8_t percentage = val->val.u8;
                if (NumericAttributeTraits<uint8_t>::IsNullValue(percentage)) {
                    return err;
                }

                app_driver_update_fan_speed(percentage);
                
            }
        }

    } else if (endpoint_id == air_quality_sensor_endpoint_id) {

    }

    return err;
}


void app_driver_event_loop() {
    ButtonEvent event;
    while (xQueueReceive(button_queue, &event, portMAX_DELAY) == pdPASS) {
        ESP_LOGI(TAG, "Button pressed (pin: %i, longPress: %i)", event.pin, event.longPress);

        if (event.longPress == false) {
            app_driver_buttons_callback(event.pin);
        } else {
            if (event.pin == BUTTON_BRIGHTNESS) {
                led_set_brightness(3);
                buzzer_beep();

                for (int i=0; i<3; i++) {
                    led_rgb_set(255,0,0);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    led_rgb_set(0,0,0);
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                
                esp_matter::factory_reset();
            }
        }
    }
} 