/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>
#include <esp_matter_cluster.h>
#include <esp_matter_feature.h> // Todo probably not needed

#include <common_macros.h>
#include <app_driver.h>
#include <app_reset.h>

#include <app/server/CommissioningWindowManager.h> 
#include <app/server/Server.h>

static const char *TAG = "app_main";
uint16_t air_purifier_endpoint_id;
uint16_t air_quality_sensor_endpoint_id;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

constexpr auto k_timeout_seconds = 300;

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
        {
            ESP_LOGI(TAG, "Fabric removed successfully");
            if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
            {
                chip::CommissioningWindowManager & commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
                constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
                if (!commissionMgr.IsCommissioningWindowOpen())
                {
                    /* After removing last fabric, this example does not remove the Wi-Fi credentials
                     * and still has IP connectivity so, only advertising on DNS-SD.
                     */
                    CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds,
                                                    chip::CommissioningWindowAdvertisement::kDnssdOnly);
                    if (err != CHIP_NO_ERROR)
                    {
                        ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
                    }
                }
            }
        break;
        }

    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
        ESP_LOGI(TAG, "Fabric will be removed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
        ESP_LOGI(TAG, "Fabric is updated");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "Fabric is committed");
        break;
    default:
        break;
    }
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

// This callback is called for every attribute update. The callback implementation shall
// handle the desired attributes and return an appropriate error code. If the attribute
// is not of your interest, please do not return an error code and strictly return ESP_OK.
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    if (type == PRE_UPDATE) {
        app_driver_attribute_update(endpoint_id, cluster_id, attribute_id, val);
    }
    return ESP_OK;
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    /* Initialize the ESP NVS layer */
    nvs_flash_init();

    /* Initialize driver */
    // removed 
    //  TODO this might be useful instead of registering the button: esp_matter::factory_reset();

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));


    // Air purifier endpoint
    air_purifier::config_t air_purifier_config;
    endpoint_t *air_purifier_endpoint = air_purifier::create(node, &air_purifier_config, ENDPOINT_FLAG_NONE, NULL);
    ABORT_APP_ON_FAILURE(air_purifier_endpoint != nullptr, ESP_LOGE(TAG, "Failed to create air purifier endpoint"));
    air_purifier_endpoint_id = endpoint::get_id(air_purifier_endpoint);
    
    // Add Multi-speed feature
    cluster_t *fan_control_cluster = cluster::get(air_purifier_endpoint, FanControl::Id);
    cluster::fan_control::feature::multi_speed::config_t multi_speed_config;
    multi_speed_config.speed_max = 100;
    cluster::fan_control::feature::multi_speed::add(fan_control_cluster, &multi_speed_config);

    // Add Automatic speed feature
    // It has no config
    cluster::fan_control::feature::fan_auto::add(fan_control_cluster);

    ESP_LOGI(TAG, "Air purifier created with endpoint_id %d", air_purifier_endpoint_id);



    // Add air quality sensor endpoint
    air_quality_sensor::config_t air_quality_sensor_config;
    endpoint_t *air_quality_sensor_endpoint = air_quality_sensor::create(node, &air_quality_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    ABORT_APP_ON_FAILURE(air_quality_sensor_endpoint != nullptr, ESP_LOGE(TAG, "Failed to add air quality cluster"));
    air_quality_sensor_endpoint_id = endpoint::get_id(air_quality_sensor_endpoint);
    ESP_LOGI(TAG, "Air quality sensor created with endpoint_id %d", air_quality_sensor_endpoint_id);

    cluster::pm25_concentration_measurement::config_t pm25_config;
    cluster_t *pm25_cluster = cluster::pm25_concentration_measurement::create(air_quality_sensor_endpoint, &pm25_config, CLUSTER_FLAG_SERVER);
    ABORT_APP_ON_FAILURE(pm25_cluster != nullptr, ESP_LOGE(TAG, "Failed to add PM2.5 cluster"));

    app_driver_hw_init();

    /* Matter start */
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    /* Starting driver with default values */
    app_driver_air_purifier_set_defaults(air_purifier_endpoint_id);
    app_driver_update_air_quality();

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::init();
#endif

    app_driver_event_loop();
}
