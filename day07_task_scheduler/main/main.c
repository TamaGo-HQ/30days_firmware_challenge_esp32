#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define LED_FAST GPIO_NUM_18
#define LED_SLOW GPIO_NUM_19
#define LED_HIGH GPIO_NUM_21

static const char *TAG = "SCHED_EX1";
TaskHandle_t task_fast_handle = NULL;

void toggle_fast(void *pvParameters) {
  while (1) {
    gpio_set_level(LED_FAST, 1);
    ESP_LOGI(TAG, "Task Fast run @ tick %lu on CPU %d", xTaskGetTickCount(),
             xPortGetCoreID());

    gpio_set_level(LED_FAST, 0);
    vTaskDelay(100);
  }
}

void toggle_slow(void *pvParameters) {
  while (1) {
    gpio_set_level(LED_SLOW, 1);
    ESP_LOGI(TAG, "Task Mid running @ tick %lu", xTaskGetTickCount());
    vTaskDelay(200);
    gpio_set_level(LED_SLOW, 0);
    vTaskDelay(200);
  }
}

void task_high(void *pvParameters) {
  while (1) {
    ESP_LOGI(TAG, "High start busy @ tick %lu on CPU %d", xTaskGetTickCount(),
             xPortGetCoreID());

    TickType_t start_tick = xTaskGetTickCount();
    TickType_t run_ticks = 500; // run for 500 ticks

    while ((xTaskGetTickCount() - start_tick) < run_ticks) {
      gpio_set_level(LED_HIGH, 1);
      taskYIELD();
      // do NOT call vTaskDelay()
      // optionally: taskYIELD() if you want to let same-priority tasks run
    }
    ESP_LOGI(TAG, "HIGH end busy @ tick %lu", xTaskGetTickCount());
    gpio_set_level(LED_HIGH, 0);
    // Optional: go to blocked state after finishing
    vTaskDelay(100); // pause before next run
  }
}

void task_controller(void *pvParameters) {
  while (1) {
    ESP_LOGI(TAG, "Controller suspending HIGH");
    vTaskSuspend(task_fast_handle); // pause high-priority task
    vTaskDelay(200);                // wait 2 sec
    ESP_LOGI(TAG, "Controller resuming HIGH");
    vTaskResume(task_fast_handle); // resume high-priority task
    vTaskDelay(200);               // wait 2 sec before next suspend
  }
}

void app_main(void) {
  gpio_config_t io_conf = {
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask =
          (1ULL << LED_FAST) | (1ULL << LED_SLOW) | (1ULL << LED_HIGH),
  };
  gpio_config(&io_conf);
  ESP_LOGI(TAG, "Started Scheduler Experiment 1");
  // xTaskCreate(task_high, "Task_High", 2048, NULL, 2, NULL);
  xTaskCreate(toggle_fast, "Task_Fast", 2048, NULL, 1, &task_fast_handle);
  xTaskCreate(task_controller, "Controller", 2048, NULL, 3, NULL);
}
