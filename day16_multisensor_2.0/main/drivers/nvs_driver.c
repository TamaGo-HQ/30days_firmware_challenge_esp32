/**
 * @file nvs_driver.c contains implementation of nvs driver functions
 */

#include "nvs_driver.h"
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"

// Default values
const app_config_t default_config = {
    .sample_ms = 1000,
    .ena_imu = 1,
    .ena_ultrason = 1
};

void init_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated or new version, erase it
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void save_config(const app_config_t *config)
{
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("app_config", NVS_READWRITE, &nvs_handle));

    // invalidate the config first
    nvs_erase_key(nvs_handle, "config_valid");  

    // write the fields one by one
    ESP_ERROR_CHECK(nvs_set_u32(nvs_handle, "sample_ms", config->sample_ms));
    ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, "ena_ultrason", config->ena_ultrason));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, "ena_imu", &config->ena_imu, sizeof(float)));

    // commit intermediate values
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));

    // signal that config is complete
    uint8_t valid = 1;
    ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, "config_valid", valid));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));

    nvs_close(nvs_handle);
}

void load_config(app_config_t *config)
{
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("app_config", NVS_READWRITE, &nvs_handle));

    uint8_t valid = 0;
    esp_err_t err = nvs_get_u8(nvs_handle, "config_valid", &valid);
    if (err != ESP_OK || valid != 1) {
        ESP_LOGW("CONFIG", "Incomplete config detected! Loading defaults.");
        *config = default_config;

        // save defaults back safely
        save_config(config);
        nvs_close(nvs_handle);
        return;
    }

    // config valid → load normally
    uint32_t sample_ms;
    nvs_get_u32(nvs_handle, "sample_ms", &sample_ms);
    config->sample_ms = (err == ESP_OK) ? sample_ms : default_config.sample_ms;

    uint8_t ena_ultrason;
    nvs_get_u8(nvs_handle, "ena_ultrason", &ena_ultrason);
    config->ena_ultrason = (err == ESP_OK) ? ena_ultrason : default_config.ena_ultrason;

    float ena_imu;
    size_t size = sizeof(float);
    nvs_get_blob(nvs_handle, "ena_imu", &ena_imu, &size);
    config->ena_imu = (err == ESP_OK) ? ena_imu : default_config.ena_imu;

    nvs_close(nvs_handle);
}
