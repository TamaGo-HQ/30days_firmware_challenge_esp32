/**
 * @file aggregator_task.c contains implementation of aggreagator task functions
 */

#include "aggregator_task.h"
#include "common/messages.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "AGGREGATOR_TASK";

static QueueHandle_t sensor_queue;
static QueueHandle_t logger_queue;

static void aggregator_task(void *arg) {
  sensor_msg_t msg;

  while (1) {
    // 1. Wait for data from any sensor
    if (xQueueReceive(sensor_queue, &msg, portMAX_DELAY) == pdTRUE) {
      // 3. Forward to logger
      if (xQueueSend(logger_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Logger queue full, dropping message");
      }
    }
  }
}

void aggregator_task_create(QueueHandle_t sensor_to_agg_q,
                            QueueHandle_t agg_to_log_q, UBaseType_t priority) {
  sensor_queue = sensor_to_agg_q;
  logger_queue = agg_to_log_q;

  xTaskCreate(aggregator_task, "aggregator_task", 2048, NULL, priority, NULL);
}