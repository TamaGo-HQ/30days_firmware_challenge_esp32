/**
 * @file nvs_driver.h contains headers for NVS memory driver
 */

#ifndef NVS_DRIVER_H
#define NVS_DRIVER_H

#include <stdio.h>

typedef struct
{
    uint32_t sample_ms;
    uint8_t ena_imu;
    uint8_t ena_ultrason;
} app_config_t;

void init_nvs();
void save_config(const app_config_t *config);
void load_config(app_config_t *config);

#endif // NVS_DRIVER_H