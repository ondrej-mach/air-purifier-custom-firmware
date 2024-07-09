/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

#include <device.h>
#include <esp_matter.h>
#include <led_driver.h>

#include <app_driver.h>
#include <fan.h>
#include <led.h>

using namespace chip::app::Clusters;
using namespace chip::app;
using namespace esp_matter;

static const char *TAG = "app_driver";
extern uint16_t air_purifier_endpoint_id;
extern uint16_t air_quality_sensor_endpoint_id;


static void app_driver_button_toggle_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Toggle button pressed");
    uint16_t endpoint_id = air_purifier_endpoint_id;
    uint32_t cluster_id = OnOff::Id;
    uint32_t attribute_id = OnOff::Attributes::OnOff::Id;

    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = cluster::get(endpoint, cluster_id);
    attribute_t *attribute = attribute::get(cluster, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
}

void app_driver_update_fan_speed(uint8_t percentage) {
    // Hardware
    fan_set_percentage(percentage);

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
}


void app_driver_report_fan_mode_from_percentage(uint8_t percentage) {
    using namespace FanControl::Attributes;

    uint16_t endpoint_id = air_purifier_endpoint_id;
    uint32_t cluster_id = FanControl::Id;

    esp_matter_attr_val_t val;
    
    if (percentage == 0) {
        val = esp_matter_enum8(static_cast<uint8_t>(FanControl::FanModeEnum::kOff));
    } else if (percentage <= 33) {
        val = esp_matter_enum8(static_cast<uint8_t>(FanControl::FanModeEnum::kLow));
    } else if (percentage <= 66) {
        val = esp_matter_enum8(static_cast<uint8_t>(FanControl::FanModeEnum::kMedium));
    } else {
        val = esp_matter_enum8(static_cast<uint8_t>(FanControl::FanModeEnum::kHigh));
    }
    
    attribute::report(endpoint_id, cluster_id, FanMode::Id, &val);
}


void app_driver_update_mode(uint8_t mode)
{
    using namespace FanControl::Attributes;

    uint16_t endpoint_id = air_purifier_endpoint_id;
    uint32_t cluster_id = FanControl::Id;

    esp_matter_attr_val_t val;
    FanControl::FanModeEnum m = static_cast<FanControl::FanModeEnum>(mode);

    uint8_t percentage = 0;

    switch (m) {
        case FanControl::FanModeEnum::kOff:
            percentage = 0;
            break;
        case FanControl::FanModeEnum::kLow:
            percentage = 10;
            break;
        case FanControl::FanModeEnum::kMedium:
            percentage = 50;
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
        fan_set_percentage(percentage);

        uint16_t endpoint_id = air_purifier_endpoint_id;
        uint32_t cluster_id = FanControl::Id;
        
        esp_matter_attr_val_t val = esp_matter_uint8(percentage);
        attribute::report(endpoint_id, cluster_id, PercentCurrent::Id, &val);
        attribute::report(endpoint_id, cluster_id, SpeedCurrent::Id, &val);
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


void app_driver_hw_init() {
    fan_init();
    led_init();
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
                app_driver_report_fan_mode_from_percentage(percentage);
            }
        }

    } else if (endpoint_id == air_quality_sensor_endpoint_id) {

    }

    return err;
}