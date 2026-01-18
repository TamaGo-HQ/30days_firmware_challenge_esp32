/**
 * @file main.c day15 flash management and nvs memory
 */

#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"

typedef struct {
    uint32_t sample_period_ms;
    uint8_t  log_enabled;
    float    calibration_factor;
} app_config_t;

// Default values
const app_config_t default_config = {
    .sample_period_ms = 1000,
    .log_enabled = 1,
    .calibration_factor = 1.0f
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

    ESP_ERROR_CHECK(nvs_set_u32(nvs_handle, "sample_ms", config->sample_period_ms));
    ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, "log_enabled", config->log_enabled));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, "cal_factor", &config->calibration_factor, sizeof(float)));

    // Commit changes
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));

    nvs_close(nvs_handle);
}

void save_config_safe(const app_config_t *config)
{
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("app_config", NVS_READWRITE, &nvs_handle));

    // invalidate the config first
    nvs_erase_key(nvs_handle, "config_valid");  

    // write the fields one by one
    ESP_ERROR_CHECK(nvs_set_u32(nvs_handle, "sample_ms", config->sample_period_ms));
    
    ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, "log_enabled", config->log_enabled));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, "cal_factor", &config->calibration_factor, sizeof(float)));

    // commit intermediate values
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));

    // signal that config is complete
    uint8_t valid = 1;
    ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, "config_valid", valid));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));

    nvs_close(nvs_handle);
}

void save_config_safe_powerloss(const app_config_t *config)
{
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("app_config", NVS_READWRITE, &nvs_handle));

    // invalidate the config first
    nvs_erase_key(nvs_handle, "config_valid");  

    // write the fields one by one
    ESP_ERROR_CHECK(nvs_set_u32(nvs_handle, "sample_ms", config->sample_period_ms));
    
    // Simulate power loss
    ESP_LOGW("CONFIG", "Simulating power loss now!");
    esp_restart();
    ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, "log_enabled", config->log_enabled));
    ESP_ERROR_CHECK(nvs_set_blob(nvs_handle, "cal_factor", &config->calibration_factor, sizeof(float)));

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
    esp_err_t err;

    // Open NVS namespace "app_config" in read-write mode
    err = nvs_open("app_config", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW("CONFIG", "Failed to open NVS, using defaults");
        *config = default_config;
        return;
    }

    // sample_period_ms
    uint32_t sample_period;
    err = nvs_get_u32(nvs_handle, "sample_ms", &sample_period);
    if (err == ESP_OK) {
        config->sample_period_ms = sample_period;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->sample_period_ms = default_config.sample_period_ms;
    } else {
        ESP_LOGE("CONFIG", "Error reading sample_period_ms");
        config->sample_period_ms = default_config.sample_period_ms;
    }

    // log_enabled
    uint8_t log_enabled;
    err = nvs_get_u8(nvs_handle, "log_enabled", &log_enabled);
    if (err == ESP_OK) {
        config->log_enabled = log_enabled;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->log_enabled = default_config.log_enabled;
    } else {
        ESP_LOGE("CONFIG", "Error reading log_enabled");
        config->log_enabled = default_config.log_enabled;
    }

    // calibration_factor
    // Store float as blob
    size_t required_size = sizeof(float);
    float cal_factor;
    err = nvs_get_blob(nvs_handle, "cal_factor", &cal_factor, &required_size);
    if (err == ESP_OK) {
        config->calibration_factor = cal_factor;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->calibration_factor = default_config.calibration_factor;
    } else {
        ESP_LOGE("CONFIG", "Error reading calibration_factor");
        config->calibration_factor = default_config.calibration_factor;
    }

    nvs_close(nvs_handle);
}

void load_config_safe(app_config_t *config)
{
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("app_config", NVS_READWRITE, &nvs_handle));

    uint8_t valid = 0;
    esp_err_t err = nvs_get_u8(nvs_handle, "config_valid", &valid);
    if (err != ESP_OK || valid != 1) {
        ESP_LOGW("CONFIG", "Incomplete config detected! Loading defaults.");
        *config = default_config;

        // Optional: save defaults back safely
        save_config_safe(config);
        nvs_close(nvs_handle);
        return;
    }

    // Config valid → load normally
    uint32_t sample_period;
    nvs_get_u32(nvs_handle, "sample_ms", &sample_period);
    config->sample_period_ms = (err == ESP_OK) ? sample_period : default_config.sample_period_ms;

    uint8_t log_enabled;
    nvs_get_u8(nvs_handle, "log_enabled", &log_enabled);
    config->log_enabled = (err == ESP_OK) ? log_enabled : default_config.log_enabled;

    float cal_factor;
    size_t size = sizeof(float);
    nvs_get_blob(nvs_handle, "cal_factor", &cal_factor, &size);
    config->calibration_factor = (err == ESP_OK) ? cal_factor : default_config.calibration_factor;

    nvs_close(nvs_handle);
}

void print_config(const app_config_t *config)
{
    ESP_LOGI("CONFIG", "sample_period_ms: %u", config->sample_period_ms);
    ESP_LOGI("CONFIG", "log_enabled: %u", config->log_enabled);
    ESP_LOGI("CONFIG", "cal_factor: %.2f", config->calibration_factor);
}


void app_main(void)
{
    // initialize NVS
    init_nvs();

    // load config (or defaults)
    app_config_t config;
    load_config_safe(&config);

    // print loaded config
    print_config(&config);

    // optional: modify a value and save it
    config.sample_period_ms += 500;  // just for testing
    save_config_safe_powerloss(&config);

    ESP_LOGI("CONFIG", "Updated config saved");
}

