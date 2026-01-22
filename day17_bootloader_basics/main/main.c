#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"

const char* ota_state_to_str(esp_ota_img_states_t state) {
    switch(state) {
        case ESP_OTA_IMG_NEW:             return "NEW";
        case ESP_OTA_IMG_PENDING_VERIFY:  return "PENDING_VERIFY";
        case ESP_OTA_IMG_VALID:           return "VALID";
        case ESP_OTA_IMG_INVALID:         return "INVALID";
        case ESP_OTA_IMG_ABORTED:         return "ABORTED";
        case ESP_OTA_IMG_UNDEFINED:       return "UNDEFINED";
        default:                          return "UNKNOWN";
    }
}

// /*  ==========first factory app to flash ========== */
// static const char *TAG = "FACTORY_APP";

// void app_main(void)
// {
//     // Get the running partition
//     const esp_partition_t *running = esp_ota_get_running_partition();
//     ESP_LOGI(TAG, "Running partition: %s", running->label);

//     // Get the boot partition (what the bootloader will select next)
//     const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
//     if (boot_partition != NULL) {
//         ESP_LOGI(TAG, "Next boot partition: %s at offset 0x%X",
//                  boot_partition->label, boot_partition->address);
//     } else {
//         ESP_LOGW(TAG, "Boot partition not found!");
//     }

//     // Infinite loop for observation
//     while (1) {
//         vTaskDelay(pdMS_TO_TICKS(2000));
//     }
// }

// /*  ========== ota 0 ========== */
// static const char *TAG = "OTA_0_APP";

// void app_main(void)
// {
//     // Get the running partition
//     const esp_partition_t *running = esp_ota_get_running_partition();
//     ESP_LOGI(TAG, "Running partition: %s", running->label);

//     // Get OTA state 
//     esp_ota_img_states_t ota_state;
//     esp_err_t err = esp_ota_get_state_partition(running, &ota_state);
//     if (err == ESP_OK) {
//          ESP_LOGI(TAG, "OTA state of running partition: %s", ota_state_to_str(ota_state));
//     } else {
//         ESP_LOGW(TAG, "No OTA state for running partition (factory or unsupported)");
//     }

//     // Get the boot partition (what the bootloader will select next)
//     const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
//     if (boot_partition != NULL) {
//         ESP_LOGI(TAG, "Next boot partition: %s at offset 0x%X",
//                  boot_partition->label, boot_partition->address);
//     } else {
//         ESP_LOGW(TAG, "Boot partition not found!");
//     }

//     // Optional: simulate app confirming itself as valid
//     esp_err_t err_validation = esp_ota_mark_app_valid_cancel_rollback();
//     if (err_validation == ESP_OK) {
//          ESP_LOGI(TAG, "OTA_0 app state validated");
//     } else {
//         ESP_LOGW(TAG, "failed to validate OTA_0 app state");
//     }

//     // Infinite loop for observation
//     while (1) {
//         vTaskDelay(pdMS_TO_TICKS(2000));
//     }
// }

// /*  ========== second factor app to flash ========== */
static const char *TAG = "FACTORY_APP_TO_OTA_0";

void app_main(void)
{
    // Get the running partition
    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition: %s", running->label);

    // Get the boot partition (what the bootloader will select next)
    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    if (boot_partition != NULL) {
        ESP_LOGI(TAG, "Next boot partition: %s at offset 0x%X",
                 boot_partition->label, boot_partition->address);
    } else {
        ESP_LOGW(TAG, "Boot partition not found!");
    }

    // Find the OTA_0 partition
    const esp_partition_t *ota_0_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);

    if (ota_0_partition != NULL) {
        ESP_LOGI(TAG, "Switching boot partition to: %s", ota_0_partition->label);

        esp_err_t res = esp_ota_set_boot_partition(ota_0_partition);
        if (res == ESP_OK) {
            ESP_LOGI(TAG, "Boot partition set successfully.");
        } else {
            ESP_LOGE(TAG, "Failed to set boot partition! err=%d", res);
        }
    } else {
        ESP_LOGE(TAG, "OTA_0 partition not found!");
    }

    // Get the boot partition (what the bootloader will select next)
    boot_partition = esp_ota_get_boot_partition();
    if (boot_partition != NULL) {
        ESP_LOGI(TAG, "Next boot partition: %s at offset 0x%X",
                 boot_partition->label, boot_partition->address);
    } else {
        ESP_LOGW(TAG, "Boot partition not found!");
    }

    // Infinite loop for observation
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}