/**
 * @file contains headers and data structures for ultrasonic sensor acquisition
 * task
 */

#ifndef ULTRASON_TASK_H
#define ULTRASON_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "drivers/ultrason_driver.h"

// queue handle provided by main.c
void ultrason_task_create(QueueHandle_t sensor_to_agg_q, UBaseType_t priority,
                          ultrason_t *sensor, uint32_t sample_ms);
void ultrason_task(QueueHandle_t sensor_to_agg_q, ultrason_t *sensor);

#endif // ULTRASON_TASK_H