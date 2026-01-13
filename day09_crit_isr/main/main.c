/**
 * @file main.c Critical Sections & ISRs
 */

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#define BUTTON_GPIO GPIO_NUM_4

static const char *TAG = "ISR_TASK";

static SemaphoreHandle_t button_semaphore;
static volatile int shared_counter = 0; // volatile so compiler doesn't optimize
portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
/*  ========== EX1 - Intro ========== */
// static void IRAM_ATTR button_isr_handler(void *arg) {
//   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//   xSemaphoreGiveFromISR(button_semaphore, &xHigherPriorityTaskWoken);
//   if (xHigherPriorityTaskWoken) {
//     portYIELD_FROM_ISR();
//   }
// }
// static void event_task(void *arg) {
//   while (1) {
//     // Block indefinitely
//     if (xSemaphoreTake(button_semaphore, portMAX_DELAY)) {
//       ESP_LOGI(TAG, "Event received from ISR");

//       ESP_LOGI(TAG, "ISR time = %lld us", isr_time);
//     }
//   }
// }

/*  ========== EX2 - Race Condition ========== */
static void IRAM_ATTR button_isr_handler(void *arg) {
  portENTER_CRITICAL_ISR(&spinlock);
  shared_counter++;
  portEXIT_CRITICAL_ISR(&spinlock);
}

static void event_task(void *arg) {
  while (1) {
    portENTER_CRITICAL(&spinlock);
    shared_counter++;
    portEXIT_CRITICAL(&spinlock);
    vTaskDelay(pdMS_TO_TICKS(1000)); // simulate work
    ESP_LOGI(TAG, "Counter = %d", shared_counter);
  }
}

static void button_init(void) {
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << BUTTON_GPIO),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_NEGEDGE, // falling edge
  };

  gpio_config(&io_conf);

  gpio_install_isr_service(0); // default ISR service
  gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);
}

void app_main(void) {

  button_semaphore = xSemaphoreCreateBinary();

  if (button_semaphore == NULL) {
    ESP_LOGE(TAG, "Failed to create semaphore");
    return;
  }

  button_init();

  xTaskCreate(event_task, "event_task", 2048, NULL, 5, NULL);

  ESP_LOGI(TAG, "System ready. Press the button.");
}
