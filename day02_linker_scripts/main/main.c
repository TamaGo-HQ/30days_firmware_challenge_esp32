/**
 * @file main.c day02 :
 */
#include "esp_log.h"
#include "esp_spi_flash.h"
#include <stdio.h>

IRAM_ATTR int fast_counter = 0;
RTC_DATA_ATTR int rtc_counter = 0;
DRAM_ATTR int dram_counter = 0;

void app_main(void) {

  /*  ========== IRAM Data ========== */
  fast_counter++;
  ESP_LOGI("TEST", "Fast counter: %d", fast_counter);
  ESP_LOGI("TEST", "Fast counter @ : %p", &fast_counter);

  /*  ========== RTC data ========== */
  rtc_counter++;
  ESP_LOGI("TEST", "RTC counter: %d", rtc_counter);
  ESP_LOGI("TEST", "RTC counter @: %p", &rtc_counter);

  /*  ========== DRAM data ========== */
  dram_counter++;
  ESP_LOGI("TEST", "DRAM counter : %d", dram_counter);
  ESP_LOGI("TEST", "DRAM counter @: %p", &dram_counter);
}
