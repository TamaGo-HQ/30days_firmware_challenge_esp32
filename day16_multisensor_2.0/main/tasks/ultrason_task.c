/**
 * @file ultrason_task.c contains implementation of ultrasonic task functions
 */

#include "ultrason_task.h"
#include "common/messages.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define ULTRASON_SAMPLE_PERIOD_MS 500

static const char *TAG = "ULTRASON_TASK";

// static QueueHandle_t ultrason_queue;
// static ultrason_t *ultrason_sensor;

void ultrason_task(QueueHandle_t sensor_to_agg_q, ultrason_t *ultrason_sensor) {
  sensor_msg_t msg;
  //uint32_t sample_ms = (uint32_t)arg;  // cast back

    // 1. Acquire sensor data
    msg.timestamp = esp_timer_get_time();
    if (!ultrason_read_data(ultrason_sensor, msg.data)) {
      ESP_LOGW(TAG, "Failed to read Ultrason");
      return;
    }

    // 2. Send to aggregator
    msg.type = SENSOR_ULTRASONIC;
    if (xQueueSend(sensor_to_agg_q, &msg, 0) != pdTRUE) {
      ESP_LOGW(TAG, "Queue full, dropping Ultrason sample");
    } else {
      //   ESP_LOGI(TAG, "ultrason data : %f %f %f %f", msg.data[0],
      //   msg.data[1],
      //            msg.data[2], msg.data[3]);
    }
}

// void ultrason_task_create(QueueHandle_t sensor_to_agg_q, UBaseType_t priority,
//                           ultrason_t *sensor, uint32_t sample_ms) {
//   ultrason_queue = sensor_to_agg_q;
//   ultrason_sensor = sensor;
//   xTaskCreate(ultrason_task, "ultrason_task", 2048,  (void *)sample_ms, priority, NULL);
// }
