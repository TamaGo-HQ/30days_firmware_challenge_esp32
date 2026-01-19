/**
 * @file logger_task.h
 */

#ifndef LOGGER_TASK_H
#define LOGGER_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void logger_task_create(QueueHandle_t logger_queue, UBaseType_t priority);

#endif // LOGGER_TASK_H