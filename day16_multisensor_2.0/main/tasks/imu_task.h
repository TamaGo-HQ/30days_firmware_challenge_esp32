/**
 * @file imu_task.h _ header for imu task creation & data structures
 */

#ifndef IMU_TASK_H
#define IMU_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "drivers/imu_driver.h"

// queue handle provided by main.c
void imu_task_create(QueueHandle_t sensor_to_agg_q, UBaseType_t priority,
                     imu_t *sensor, uint32_t sample_ms);

void imu_task(QueueHandle_t sensor_to_agg_q, imu_t *sensor);
#endif // IMU_TASK_h
