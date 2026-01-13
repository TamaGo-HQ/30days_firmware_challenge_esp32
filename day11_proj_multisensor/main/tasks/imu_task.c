/**
 * @file imu_task.d source file for imu task
 */

#include "imu_task.h"
#include "common/messages.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sensors/imu_driver.h" // Your HAL driver for IMU

static const char *TAG = "IMU_TASK";

static QueueHandle_t imu_queue;
static imu_t *imu_sensor;

#define IMU_SAMPLE_PERIOD_MS 500 // 20 Hz

// Task function
static void imu_task(void *arg) {
  sensor_msg_t msg;

  while (1) {
    // 1. Acquire sensor data
    msg.timestamp = esp_timer_get_time();
    if (!imu_read_data(imu_sensor, msg.data)) {
      ESP_LOGW(TAG, "Failed to read IMU");
      vTaskDelay(pdMS_TO_TICKS(IMU_SAMPLE_PERIOD_MS));
      continue;
    }

    // 2. Send to aggregator
    msg.type = SENSOR_IMU;
    if (xQueueSend(imu_queue, &msg, 0) != pdTRUE) {
      ESP_LOGW(TAG, "Queue full, dropping IMU sample");
    } else {
      // ESP_LOGI(TAG, "IMU data : %f %f %f %f", msg.data[0], msg.data[1],
      //   msg.data[2], msg.data[3]);
    }

    // 3. Wait until next sample
    vTaskDelay(pdMS_TO_TICKS(IMU_SAMPLE_PERIOD_MS));
  }
}

// Public creation function
void imu_task_create(QueueHandle_t sensor_to_agg_q, UBaseType_t priority,
                     imu_t *sensor) {
  imu_queue = sensor_to_agg_q;
  imu_sensor = sensor;
  xTaskCreate(imu_task, "imu_task", 2048, NULL, priority, NULL);
}
