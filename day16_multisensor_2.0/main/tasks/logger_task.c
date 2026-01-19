/**
 * @file logger_task.h
 */

#include "logger_task.h"
#include "common/messages.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "LOGGER";

static QueueHandle_t s_logger_queue;

static void logger_task(void *arg) {
  sensor_msg_t msg;

  while (1) {
    // Block forever waiting for data
    if (xQueueReceive(s_logger_queue, &msg, portMAX_DELAY) == pdTRUE) {

      switch (msg.type) {

      case SENSOR_IMU:
        ESP_LOGI(TAG, "[IMU] ts=%lld | ax=%.2f ay=%.2f az=%.2f", msg.timestamp,
                 msg.data[0], msg.data[1], msg.data[2]);
        break;

      case SENSOR_ULTRASONIC:
        ESP_LOGI(TAG, "[ULTRA] ts=%lld | distance=%.2f cm", msg.timestamp,
                 msg.data[0]);
        break;

      default:
        ESP_LOGW(TAG, "Unknown sensor type (%d)", msg.type);
        break;
      }
    }
  }
}

void logger_task_create(QueueHandle_t logger_queue, UBaseType_t priority) {
  s_logger_queue = logger_queue;

  xTaskCreate(logger_task, "logger_task",
              4096, // Logging needs stack (printf, formatting)
              NULL,
              priority, // LOW priority
              NULL);
}