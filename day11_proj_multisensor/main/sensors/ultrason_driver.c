/**
 * @file ultrason_driver.c contains implementation of ultrasonic sensor driver
 * functions
 */
// ultrasonic_driver.c

#include "ultrason_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ULTRASON_DRIVER";

bool ultrason_init(ultrason_t *sensor) {
  if (!sensor)
    return false;

  // Trigger pin: output
  gpio_config_t io_conf = {.pin_bit_mask = 1ULL << sensor->trig_pin,
                           .mode = GPIO_MODE_OUTPUT,
                           .pull_up_en = GPIO_PULLUP_DISABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);
  gpio_set_level(sensor->trig_pin, 0); // idle low

  // Echo pin: input
  io_conf.pin_bit_mask = 1ULL << sensor->echo_pin;
  io_conf.mode = GPIO_MODE_INPUT;
  gpio_config(&io_conf);

  ESP_LOGI(TAG, "Ultrasonic sensor initialized: TRIG=%d ECHO=%d",
           sensor->trig_pin, sensor->echo_pin);

  return true;
}

bool ultrason_read_data(ultrason_t *sensor, float *distance) {
  if (!sensor || !distance)
    return false;

  // Trigger a 10µs pulse
  gpio_set_level(sensor->trig_pin, 1);
  esp_rom_delay_us(10);
  gpio_set_level(sensor->trig_pin, 0);

  // Wait for echo pin to go HIGH
  int64_t start_time = esp_timer_get_time();
  int64_t timeout = 100000; // 30ms timeout (~5m distance)
  while (gpio_get_level(sensor->echo_pin) == 0) {
    if ((esp_timer_get_time() - start_time) > timeout) {
      ESP_LOGE(TAG, "Echo timeout");
      return false;
    }
  }

  // Measure duration of HIGH pulse
  int64_t echo_start = esp_timer_get_time();
  while (gpio_get_level(sensor->echo_pin) == 1) {
    if ((esp_timer_get_time() - echo_start) > timeout) {
      ESP_LOGW(TAG, "Echo pulse too long");
      return false;
    }
  }
  int64_t echo_end = esp_timer_get_time();

  int64_t pulse_duration_us = echo_end - echo_start;

  // Convert to cm: speed of sound ~ 343 m/s
  *distance = (pulse_duration_us / 2.0f) * 0.0343f;

  return true;
}
