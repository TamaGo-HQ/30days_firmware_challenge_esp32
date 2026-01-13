/**
 * @file imu_driver.h filce containing headers and structures for the IMU driver
 */

#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H

#include "driver/gpio.h"
#include "driver/i2c.h"
#include <stdbool.h>

typedef struct {
  int i2c_addr;
  gpio_num_t sda_pin;
  gpio_num_t scl_pin;
} imu_t;

// initialize IMU
bool imu_init(imu_t *sensor);

void i2c_scan(i2c_port_t i2c_num);

// read data from sepecified IMU
bool imu_read_data(imu_t *sensor, float *data);

#endif // IMU_DRIVER_H