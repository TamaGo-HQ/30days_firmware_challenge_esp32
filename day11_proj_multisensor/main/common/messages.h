/**
 * @file messages.h _ data structure shared between sensor & other task
 */
#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdio.h>

typedef enum { SENSOR_IMU, SENSOR_ULTRASONIC } sensor_type_t;

typedef struct {
  sensor_type_t type;
  int64_t timestamp; // acquisition time
  float data[4];
} sensor_msg_t;
#endif // MESSAGES_H
