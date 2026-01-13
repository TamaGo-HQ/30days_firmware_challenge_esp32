/**
 * @file main.c day11 project multisensor acquisition
 */
#include "common/messages.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensors/imu_driver.h"
#include "sensors/ultrason_driver.h"
#include "tasks/aggregator_task.h"
#include "tasks/imu_task.h"
#include "tasks/logger_task.h"
#include "tasks/ultrason_task.h"
#include <stdio.h>

#define SENSOR_TO_AGG_Q_LEN 12
#define AGG_TO_LOG_Q_LEN 50

QueueHandle_t sensor_to_agg_q;
QueueHandle_t agg_to_log_q;

void app_main(void) {
  // create queues
  sensor_to_agg_q = xQueueCreate(SENSOR_TO_AGG_Q_LEN, sizeof(sensor_msg_t));
  agg_to_log_q = xQueueCreate(AGG_TO_LOG_Q_LEN, sizeof(sensor_msg_t));

  // define IMU struct
  static imu_t imu1 = {
      .i2c_addr = 0x68, .sda_pin = GPIO_NUM_21, .scl_pin = GPIO_NUM_22};
  // define ultrason struct
  static ultrason_t ultrason1 = {.trig_pin = GPIO_NUM_16,
                                 .echo_pin = GPIO_NUM_17};

  // create IMU task
  imu_init(&imu1);
  i2c_scan(I2C_NUM_0);
  imu_task_create(sensor_to_agg_q, 8, &imu1);
  ultrason_init(&ultrason1);
  ultrason_task_create(sensor_to_agg_q, 8, &ultrason1);
  aggregator_task_create(sensor_to_agg_q, agg_to_log_q, 7);
  logger_task_create(agg_to_log_q, 1);
}
