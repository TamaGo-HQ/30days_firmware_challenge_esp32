/**
 * @file main.c day11 project multisensor acquisition
 */
#include "common/messages.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "drivers/imu_driver.h"
#include "drivers/ultrason_driver.h"
#include "tasks/aggregator_task.h"
#include "tasks/imu_task.h"
#include "tasks/logger_task.h"
#include "tasks/ultrason_task.h"
#include <stdio.h>
#include "drivers/nvs_driver.h"
#include "esp_sleep.h"
#include "esp_log.h"

#define SENSOR_TO_AGG_Q_LEN 12
#define AGG_TO_LOG_Q_LEN 50

#define LED_GPIO   GPIO_NUM_4

static const char *TAG = "SLEEP";
static uint32_t sample_ms;

QueueHandle_t sensor_to_agg_q;
QueueHandle_t agg_to_log_q;

app_config_t app_config;

void go_to_sleep(uint32_t sleep_ms) {
    ESP_LOGI(TAG, "Entering deep sleep for %u ms", sleep_ms);
    esp_sleep_enable_timer_wakeup(sleep_ms * 1000ULL); // microseconds
    esp_deep_sleep_start();
}

void led_init(void)
{
    gpio_config_t io_conf = {0};

    io_conf.pin_bit_mask = (1ULL << LED_GPIO);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    (void)gpio_config(&io_conf);

    /* Ensure LED is OFF after init */
    (void)gpio_set_level(LED_GPIO, 0);
}

void app_main(void) {

  gpio_set_level(LED_GPIO, 1);

  // create queues
  sensor_to_agg_q = xQueueCreate(SENSOR_TO_AGG_Q_LEN, sizeof(sensor_msg_t));
  agg_to_log_q = xQueueCreate(AGG_TO_LOG_Q_LEN, sizeof(sensor_msg_t));

  aggregator_task_create(sensor_to_agg_q, agg_to_log_q, 7);
  logger_task_create(agg_to_log_q, 1);


  // define sensors
  static imu_t imu1 = {
      .i2c_addr = 0x68, .sda_pin = GPIO_NUM_21, .scl_pin = GPIO_NUM_22};
  // define ultrason struct
  static ultrason_t ultrason1 = {.trig_pin = GPIO_NUM_16,
                                 .echo_pin = GPIO_NUM_17};

  init_nvs();
  led_init();
  ultrason_init(&ultrason1);
  imu_init(&imu1);

  load_config(&app_config);
  sample_ms = app_config.sample_ms;

  imu_task(sensor_to_agg_q, &imu1);
  ultrason_task(sensor_to_agg_q, &ultrason1);

  gpio_set_level(LED_GPIO, 0);
  go_to_sleep(sample_ms); // enter deep sleep
}
