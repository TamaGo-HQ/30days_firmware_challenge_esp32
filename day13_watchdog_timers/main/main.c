/**
 * @file main.c day13 watchodog timers
 */

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define BUTTON_GPIO 4

static const char *TAG = "ISR_WDT";

void IRAM_ATTR button_isr_handler(void *arg) {
  // INTENTIONALLY BAD: long blocking loop
  for (volatile int i = 0; i < 100000000; i++) {
    // do nothing, just waste cycles
  }
}

void init_button() {
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << BUTTON_GPIO),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = 1, // enable internal pull-up
      .pull_down_en = 0,
      .intr_type = GPIO_INTR_NEGEDGE // falling edge (button press)
  };
  gpio_config(&io_conf);

  // Install ISR service
  gpio_install_isr_service(0);
  gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);
}

void bad_task(void *pvParameters) {
  esp_task_wdt_add(NULL);

  while (1) {
    esp_task_wdt_reset(); // Feed the watchdog
  }
}

void app_main(void) {
  ESP_LOGI("MAIN", "Starting watchdog demo");

  /*  ========== Task Watchdog Timer ========== */
  // xTaskCreate(bad_task, "bad_task", 2048, NULL, 5, NULL);

  /*  ========== Interrupt Watchdog Timer ========== */
  init_button();

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Main task alive");
  }
}
