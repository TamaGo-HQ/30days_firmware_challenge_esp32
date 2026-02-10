#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>
#include "esp_timer.h"


#define I2C_MASTER_SCL_IO           22      // change to your GPIO
#define I2C_MASTER_SDA_IO           21      // change to your GPIO
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ           100000
#define I2C_MASTER_TX_BUF_DISABLE    0
#define I2C_MASTER_RX_BUF_DISABLE    0

#define WHO_AM_I_REG    0x75
#define IMU_ADDR                    0x68      // 7-bit IMU address
#define ACCEL_START_REG             0x3B      // Starting register for accelerometer (XH)
#define READ_LEN                     14        // Read ACC Temp and Gyro data
#define PWR_MGMT_1 0x6B
#define GYRO_START_OFFSET 8  // gyro data starts at raw[8]

static const char *TAG = "I2C_SCAN";

esp_err_t imu_read_who_am_i(uint8_t *who_am_i)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // START
    i2c_master_start(cmd);

    // Address + Write
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_WRITE, true);

    // Register address
    i2c_master_write_byte(cmd, WHO_AM_I_REG, true);

    // Repeated START
    i2c_master_start(cmd);

    // Address + Read
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_READ, true);

    // Read 1 byte, then NACK
    i2c_master_read_byte(cmd, who_am_i, I2C_MASTER_NACK);

    // STOP
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(
        I2C_MASTER_NUM,
        cmd,
        1000 / portTICK_PERIOD_MS
    );

    i2c_cmd_link_delete(cmd);
    return ret;
}

void i2c_master_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                       I2C_MASTER_RX_BUF_DISABLE,
                       I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t i2c_probe(uint8_t addr)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t imu_read_bytes(uint8_t start_reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // START + write phase
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, start_reg, true);

    // Repeated START + read phase
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_READ, true);

    // Read multiple bytes
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);

    // STOP
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(
        I2C_MASTER_NUM,
        cmd,
        1000 / portTICK_PERIOD_MS
    );

    i2c_cmd_link_delete(cmd);
    return ret;
}

void imu_wake_up(void)
{
    uint8_t data = 0x00; // clear sleep bit
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (IMU_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, PWR_MGMT_1, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void imu_logger_task(void *arg)
{
    const int LOG_TIME_MS   = 15000;
    const int PAUSE_TIME_MS = 5000;
    const int SEGMENTS      = 4;
    uint8_t raw[READ_LEN];
    for (int seg = 0; seg < SEGMENTS; seg++) {

        int64_t start_time = esp_timer_get_time() / 1000;

        while ((esp_timer_get_time() / 1000 - start_time) < LOG_TIME_MS) {

             esp_err_t ret = imu_read_bytes(ACCEL_START_REG, raw, READ_LEN);
             if (ret == ESP_OK) {

                // -------- Accelerometer --------
                int16_t ax = (raw[0] << 8) | raw[1];
                int16_t ay = (raw[2] << 8) | raw[3];
                int16_t az = (raw[4] << 8) | raw[5];

                float ax_g = ax / 16384.0f;
                float ay_g = ay / 16384.0f;
                float az_g = az / 16384.0f;

                // -------- Gyroscope --------
                int16_t gx = (raw[8]  << 8) | raw[9];
                int16_t gy = (raw[10] << 8) | raw[11];
                int16_t gz = (raw[12] << 8) | raw[13];

                float gx_dps = gx / 131.0f;
                float gy_dps = gy / 131.0f;
                float gz_dps = gz / 131.0f;

                float accel_mag = sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
                float gyro_mag  = sqrtf(gx_dps*gx_dps + gy_dps*gy_dps + gz_dps*gz_dps);
                int64_t time_ms = esp_timer_get_time() / 1000;

                // CSV output
                printf("%lld,%.3f,%.3f\n", time_ms, accel_mag, gyro_mag);}

            vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
        }

        printf("---done---\n");

        if (seg < SEGMENTS - 1) {
            vTaskDelay(pdMS_TO_TICKS(PAUSE_TIME_MS));
        }
    }

    // Stop task forever
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing I2C...");
    i2c_master_init();
    imu_wake_up();
    // uint8_t raw[READ_LEN];
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            ESP_LOGI("I2C_SCAN", "Found device at 0x%02X", addr);
    }
    }
    esp_log_level_set("*", ESP_LOG_NONE);
    xTaskCreate(
        imu_logger_task,     // task function
        "imu_logger",        // name
        4096,                // stack size
        NULL,                // parameters
        5,                   // priority
        NULL                 // task handle
    );


    // while (1) {
    //     esp_err_t ret = imu_read_bytes(ACCEL_START_REG, raw, READ_LEN);
    //     if (ret == ESP_OK) {

    //         // -------- Accelerometer --------
    //         int16_t ax = (raw[0] << 8) | raw[1];
    //         int16_t ay = (raw[2] << 8) | raw[3];
    //         int16_t az = (raw[4] << 8) | raw[5];

    //         float ax_g = ax / 16384.0f;
    //         float ay_g = ay / 16384.0f;
    //         float az_g = az / 16384.0f;

    //         // -------- Gyroscope --------
    //         int16_t gx = (raw[8]  << 8) | raw[9];
    //         int16_t gy = (raw[10] << 8) | raw[11];
    //         int16_t gz = (raw[12] << 8) | raw[13];

    //         float gx_dps = gx / 131.0f;
    //         float gy_dps = gy / 131.0f;
    //         float gz_dps = gz / 131.0f;

    //         // Accelerometer magnitude (in g)
    //         float accel_mag = sqrtf(
    //             ax_g * ax_g +
    //             ay_g * ay_g +
    //             az_g * az_g
    //         );

    //         // Gyroscope magnitude (in °/s)
    //         float gyro_mag = sqrtf(
    //             gx_dps * gx_dps +
    //             gy_dps * gy_dps +
    //             gz_dps * gz_dps
    //         );

    //         int64_t timestamp_ms = esp_timer_get_time() / 1000;

    //         printf("%lld,%.4f,%.4f\n", timestamp_ms, accel_mag, gyro_mag);
    //     } else {
    //         ESP_LOGE(TAG, "Failed to read IMU: %s", esp_err_to_name(ret));
    //     }

    //     vTaskDelay(pdMS_TO_TICKS(10)); // keep same rate for now
    // }
}
