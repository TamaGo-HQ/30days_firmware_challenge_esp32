/**
 * @file aggregator_task.h contains headers and data structor for the aggregator
 * task
 */

#ifndef AGGREGATOR_TASK_H
#define AGGREGATOR_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void aggregator_task_create(QueueHandle_t sensor_to_agg_q,
                            QueueHandle_t agg_to_log_q, UBaseType_t priority);

#endif // AGGREGATOR_TASK_H