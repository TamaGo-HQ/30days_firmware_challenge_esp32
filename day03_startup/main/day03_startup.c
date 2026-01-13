/**
 * @file day03 learning how the board starts up
 */

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include <stdio.h>

/*  ========== OTA_0 app ========== */
/*
 *  Build and flash at offset 0x(OTA_0 offset)
 */
void app_main(void) {
  const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);

  ESP_LOGI("OTA", "From OTA_0 Switching to partition: %s", next->label);
  esp_ota_set_boot_partition(next);

  ESP_LOGI("OTA", "Rebooting...");
  esp_restart();
}

/*  ========== OTA_1 app ========== */
/*
 *  Build and flash at offset 0x(OTA_1 offset)
 */
// void app_main(void) {
//   const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);

//   ESP_LOGI("OTA", "From OTA_1 Switching to partition: %s", next->label);
//   esp_ota_set_boot_partition(next);

//   ESP_LOGI("OTA", "Rebooting...");
//   esp_restart();
// }
