/**
 * @file imu_driver.c contains implementation of imu driver
 */

#include "imu_driver.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "IMU_DRIVER";

// MPU-6050 registers
#define MPU_ADDR(addr) ((addr) << 1) // ESP-IDF shifts 1 bit
#define WHO_AM_I 0x75
#define PWR_MGMT_1 0x6B
#define GYRO_XOUT_H 0x43
#define ACCEL_XOUT_H 0x3B
#define TEMP_OUT_H 0x41

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_TIMEOUT_MS 1000

void i2c_scan(i2c_port_t i2c_num) {
  ESP_LOGI(TAG, "Scanning I2C bus %d...\n", i2c_num);
  for (uint8_t addr = 1; addr < 127; addr++) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "Device found at 0x%02x\n", addr);
    }
  }
}

// Initialize I2C bus and IMU
bool imu_init(imu_t *sensor) {
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = sensor->sda_pin,
      .scl_io_num = sensor->scl_pin,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = 100000 // 100 kHz for stability
  };
  i2c_param_config(I2C_MASTER_NUM, &conf);
  i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

  uint8_t data = 0;

  // 1️⃣ Write PWR_MGMT_1 register address
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, MPU_ADDR(sensor->i2c_addr) | I2C_MASTER_WRITE,
                        true);
  i2c_master_write_byte(cmd, PWR_MGMT_1, true);
  i2c_master_stop(cmd);
  if (i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS) !=
      ESP_OK) {
    ESP_LOGE(TAG, "Failed to select PWR_MGMT_1 register");
    i2c_cmd_link_delete(cmd);
    return false;
  }
  i2c_cmd_link_delete(cmd);

  // 2️⃣ Read 1 byte from that register
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, MPU_ADDR(sensor->i2c_addr) | I2C_MASTER_READ,
                        true);
  i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);
  i2c_master_stop(cmd);
  if (i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS) !=
      ESP_OK) {
    ESP_LOGE(TAG, "Failed to read PWR_MGMT_1");
    i2c_cmd_link_delete(cmd);
    return false;
  }
  i2c_cmd_link_delete(cmd);

  // Clear sleep bit
  data &= ~0x40;

  // 3️⃣ Write back to PWR_MGMT_1 to wake IMU
  uint8_t write_data[2] = {PWR_MGMT_1, data};
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, MPU_ADDR(sensor->i2c_addr) | I2C_MASTER_WRITE,
                        true);
  i2c_master_write(cmd, write_data, 2, true);
  i2c_master_stop(cmd);
  if (i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS) !=
      ESP_OK) {
    ESP_LOGE(TAG, "Failed to wake up IMU");
    i2c_cmd_link_delete(cmd);
    return false;
  }
  i2c_cmd_link_delete(cmd);

  ESP_LOGI(TAG, "IMU initialized at address 0x%02x", sensor->i2c_addr);
  return true;
}

// Read gyro data
bool imu_read_data(imu_t *sensor, float *data) {
  uint8_t buf[6];
  uint8_t reg_addr = GYRO_XOUT_H;

  // Write register first
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, MPU_ADDR(sensor->i2c_addr) | I2C_MASTER_WRITE,
                        true);
  i2c_master_write_byte(cmd, reg_addr, true);
  i2c_master_stop(cmd);
  if (i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS) !=
      ESP_OK) {
    ESP_LOGE(TAG, "Failed to select GYRO_XOUT_H register");
    i2c_cmd_link_delete(cmd);
    return false;
  }
  i2c_cmd_link_delete(cmd);

  // Read 6 bytes
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, MPU_ADDR(sensor->i2c_addr) | I2C_MASTER_READ,
                        true);
  i2c_master_read(cmd, buf, 6, I2C_MASTER_LAST_NACK);
  i2c_master_stop(cmd);
  if (i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS) !=
      ESP_OK) {
    ESP_LOGE(TAG, "Failed to read gyro");
    i2c_cmd_link_delete(cmd);
    return false;
  }
  i2c_cmd_link_delete(cmd);

  int16_t gx = (buf[0] << 8) | buf[1];
  int16_t gy = (buf[2] << 8) | buf[3];
  int16_t gz = (buf[4] << 8) | buf[5];

  data[0] = gx / 131.0f;
  data[1] = gy / 131.0f;
  data[2] = gz / 131.0f;
  data[3] = 0;

  return true;
}