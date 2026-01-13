/**
 * @file ultrason_driver.h contains headers and data structures for ultrasonic
 * sensor driver
 */

#ifndef ULTRASON_DRIVER_H
#define ULTRASON_DRIVER_H

#include "driver/gpio.h"

typedef struct {
  gpio_num_t trig_pin;
  gpio_num_t echo_pin;
} ultrason_t;

// initialize ULTRASONIC
bool ultrason_init(ultrason_t *sensor);

// read data from sepecified IMU
bool ultrason_read_data(ultrason_t *sensor, float *data);

#endif // ULTRASON_DRIVER_H