/**
 * @file main.c day08 Learning Inter Task Communication
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>

#define QUEUE_LENGTH 3
#define QUEUE_ITEM_SIZE sizeof(int)

static const char *TAG = "PROD_CONS";

QueueHandle_t intQueue;
SemaphoreHandle_t binarySem;

SemaphoreHandle_t resource; // can be mutex or binary semaphore

// Producer task
void producer_task(void *pvParameters) {
  int counter = 0;
  while (1) {
    // Try to send item with 100 ticks timeout
    if (xQueueSend(intQueue, &counter, 10) == pdPASS) {
      ESP_LOGI(TAG, "Producer running @ tick %lu Produced: %d",
               xTaskGetTickCount(), counter);
    } else {
      ESP_LOGW(TAG, "Queue full! Could not send: %d", counter);
    }
    vTaskDelay(50); // produce every 50ticks
    counter++;
  }
}

// Consumer task
void consumer_task(void *pvParameters) {
  int receivedValue;
  while (1) {
    // Block indefinitely until an item is available
    if (xQueueReceive(intQueue, &receivedValue, portMAX_DELAY) == pdPASS) {
      ESP_LOGI(TAG, "Consumer running @ tick %lu Consumed: %d",
               xTaskGetTickCount(), receivedValue);
      vTaskDelay(100);
    }
  }
}

// Task A: simulates an interrupt or event generator
void taskA_simulatedISR(void *pvParameters) {
  while (1) {
    // Give the semaphore to signal Task B
    if (xSemaphoreGive(binarySem) == pdTRUE) {
      ESP_LOGI(TAG, "@%lu Task A: Event signaled (Semaphore given)",
               xTaskGetTickCount());
    } else {
      ESP_LOGW(TAG, "@%lu Task A: Semaphore already full", xTaskGetTickCount());
    }
    ESP_LOGI(TAG,
             "@%lu Task A: will this appear before or after taskB handling?",
             xTaskGetTickCount());
    vTaskDelay(pdMS_TO_TICKS(1000)); // simulate event every 1 second
  }
}

void taskB_waiter(void *pvParameters) {
  while (1) {
    ESP_LOGI(TAG, "@%lu Task B: Waiting for event...", xTaskGetTickCount());
    // Block indefinitely until semaphore is given
    if (xSemaphoreTake(binarySem, portMAX_DELAY) == pdTRUE) {
      ESP_LOGI(TAG, "@%lu Task B: Event received! Handling it...",
               xTaskGetTickCount());
    }
  }
}

void low_task(void *pvParameters) {
  while (1) {
    ESP_LOGI(TAG, "Low task started, trying to take resource");
    xSemaphoreTake(resource, portMAX_DELAY);
    ESP_LOGI(TAG, "Low task acquired resource, working... @%d",
             xPortGetCoreID());

    // Simulate long work
    vTaskDelay(200);

    ESP_LOGI(TAG, "Low task releasing resource @%lu, @%d", xTaskGetTickCount(),
             xPortGetCoreID());
    xSemaphoreGive(resource);
    vTaskDelay(10000);
  }
}

void medium_task(void *pvParameters) {
  while (1) {
    /* code */

    ESP_LOGI(TAG, "Medium task started, CPU hog @%d", xPortGetCoreID());

    TickType_t start_tick = xTaskGetTickCount();
    TickType_t run_ticks = 300; // run for 500 ticks

    while ((xTaskGetTickCount() - start_tick) < run_ticks) {
      // do NOT call vTaskDelay()
      // optionally: taskYIELD() if you want to let same-priority tasks run
    }
    ESP_LOGI(TAG, "Medium task done , CPU hog @%lu", xTaskGetTickCount());
    vTaskDelay(10000);
  }
}

void high_task(void *pvParameters) {
  while (1) {
    /* code */

    vTaskDelay(pdMS_TO_TICKS(100)); // wait a bit to let low task acquire
    ESP_LOGI(TAG, "High task started, trying to take resource @%lu @%d",
             xTaskGetTickCount(), xPortGetCoreID());

    xSemaphoreTake(resource, portMAX_DELAY);

    ESP_LOGI(TAG, "High task acquired resource @%lu @%d", xTaskGetTickCount(),
             xPortGetCoreID());
    xSemaphoreGive(resource);
    vTaskDelay(10000);
  }
}

TaskHandle_t taskBHandle = NULL;

// Task A: notifier
void taskA(void *pvParameters) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "@%lu Task A: notifying", xTaskGetTickCount());

    xTaskNotifyGive(taskBHandle);
  }
}

// Task B: waits for notification
void taskB(void *pvParameters) {
  while (1) {
    ESP_LOGI(TAG, "@%lu Task B: waiting", xTaskGetTickCount());

    // Block until notification arrives
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    ESP_LOGI(TAG, "@%lu Task B: notified!", xTaskGetTickCount());
  }
}

void app_main(void) {
  /*  ========== Queue ========== */
  //   intQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
  //   if (intQueue == NULL) {
  //     ESP_LOGE(TAG, "Failed to create Queue");
  //     return;
  //   }

  //   xTaskCreate(producer_task, "Producer", 2048, NULL, 1, NULL);
  //   xTaskCreate(consumer_task, "Consumer", 2048, NULL, 1, NULL);

  //   ESP_LOGI(TAG, "Producer/Consumer tasks started");

  /*  ========== Binary Semaphore ========== */
  // Create binary semaphore
  //   binarySem = xSemaphoreCreateBinary();
  //   if (binarySem == NULL) {
  //     ESP_LOGE(TAG, "Failed to create binary semaphore");
  //     return;
  //   }

  //   // Create tasks
  //   xTaskCreate(taskA_simulatedISR, "TaskA_ISR", 2048, NULL, 2, NULL);
  //   xTaskCreate(taskB_waiter, "TaskB_Waiter", 2048, NULL, 1, NULL);

  //   ESP_LOGI(TAG, "Binary semaphore event demo started");

  /*  ========== semaphores vs mutexe ========== */
  // ESP_LOGI(TAG, "Priority inversion demo starting");

  // --- Use binary semaphore first ---
  //   ESP_LOGI(TAG, "Using binary semaphore");
  //   resource = xSemaphoreCreateBinary();
  //   if (resource == NULL)
  //     return;

  //   // Binary semaphore must be given initially to allow first take
  //   xSemaphoreGive(resource);

  //   xTaskCreatePinnedToCore(low_task, "Low", 2048, NULL, 1, NULL, 0);
  //   xTaskCreatePinnedToCore(medium_task, "Medium", 2048, NULL, 2, NULL, 0);
  //   xTaskCreatePinnedToCore(high_task, "High", 2048, NULL, 3, NULL, 0);

  //-- -Now use mutex-- - ESP_LOGI(TAG, "Switching to mutex");
  //   resource = xSemaphoreCreateMutex();
  //   if (resource == NULL)
  //     return;

  //   xTaskCreatePinnedToCore(low_task, "Low", 2048, NULL, 1, NULL, 0);
  //   xTaskCreatePinnedToCore(medium_task, "Medium", 2048, NULL, 2, NULL, 0);
  //   xTaskCreatePinnedToCore(high_task, "High", 2048, NULL, 3, NULL, 0);

  xTaskCreate(taskB, "TaskB", 2048, NULL, 1, &taskBHandle);
  xTaskCreate(taskA, "TaskA", 2048, NULL, 2, NULL);
}
