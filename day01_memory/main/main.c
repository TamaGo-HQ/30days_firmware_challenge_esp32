/**
 * @file main.c - ESP32 memory experimentd
 */

#include "driver/gpio.h"
#include "driver/i2s.h"
#include "driver/uart.h"
#include "esp_heap_caps.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_system.h"
#include "esp_cache.h"

#define LED_GPIO 4
#define I2S_NUM I2S_NUM_0
#define SAMPLE_RATE 44100
#define BUF_SIZE 1024
#define LOOP_COUNT 1000000

int normal_var = 0;            // DRAM
RTC_DATA_ATTR int rtc_var = 0; // RTC slow memory

void blink_led_task(void *pvParameter) {
  for (int i = 0; i < 5; i++) { // blink 5 times before deep sleep
    gpio_set_level(LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(LED_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Increment variables
    normal_var++;
    rtc_var++;

    printf("During run: normal_var=%d, rtc_var=%d\n", normal_var, rtc_var);
    printf("normal_var address: %p\n", &normal_var);
    printf("rtc_var address: %p\n", &rtc_var);
  }

  printf("Entering deep sleep for 5 seconds...\n");
  // Configure wakeup timer
  esp_sleep_enable_timer_wakeup(5000000); // 5 seconds in microseconds
  esp_deep_sleep_start(); // CPU stops here, will reset after wakeup
}

void i2s_dma_example() {
  // I2S configuration (simple TX)
  i2s_config_t i2s_config = {.mode = I2S_MODE_MASTER | I2S_MODE_TX,
                             .sample_rate = SAMPLE_RATE,
                             .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                             .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
                             .communication_format = I2S_COMM_FORMAT_I2S_MSB,
                             .intr_alloc_flags = 0,
                             .dma_buf_count = 4,
                             .dma_buf_len = 256,
                             .use_apll = false,
                             .tx_desc_auto_clear = true,
                             .fixed_mclk = 0};

  i2s_pin_config_t pin_config = {.bck_io_num = 26,
                                 .ws_io_num = 25,
                                 .data_out_num = 22,
                                 .data_in_num = I2S_PIN_NO_CHANGE};

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);

  // Allocate buffers
  uint16_t *dma_buf =
      heap_caps_malloc(BUF_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);
  uint16_t *dram_buf =
      heap_caps_malloc(BUF_SIZE * sizeof(uint16_t), MALLOC_CAP_DEFAULT);

  if (!dma_buf || !dram_buf) {
    printf("Buffer allocation failed!\n");
    return;
  }

  // Fill buffers with dummy waveform
  for (int i = 0; i < BUF_SIZE; i++) {
    dma_buf[i] = i % 32768;
    dram_buf[i] = i % 32768;
  }

  size_t bytes_written;
  int64_t t0, t1;

  // Write DMA buffer
  t0 = esp_timer_get_time();
  i2s_write(I2S_NUM, dma_buf, BUF_SIZE * sizeof(uint16_t), &bytes_written,
            portMAX_DELAY);
  t1 = esp_timer_get_time();
  printf("I2S write (DMA buffer) took %lld us\n", t1 - t0);

  // Write normal DRAM buffer
  t0 = esp_timer_get_time();
  i2s_write(I2S_NUM, dram_buf, BUF_SIZE * sizeof(uint16_t), &bytes_written,
            portMAX_DELAY);
  t1 = esp_timer_get_time();
  printf("I2S write (normal DRAM) took %lld us\n", t1 - t0);

  // Clean up
  free(dma_buf);
  free(dram_buf);
  i2s_driver_uninstall(I2S_NUM);
}

// Function in flash (XIP)
void flash_loop(void) {
  for (volatile int i = 0; i < LOOP_COUNT; i++)
    ;
}
// Function in IRAM
void IRAM_ATTR iram_loop(void) {
  for (volatile int i = 0; i < LOOP_COUNT; i++)
    ;
}

void cache_timing_experiment(void) {
  int64_t start, end;

  start = esp_timer_get_time();
  flash_loop();
  end = esp_timer_get_time();
  printf("Flash loop time: %lld us\n", end - start);

  start = esp_timer_get_time();
  iram_loop();
  end = esp_timer_get_time();
  printf("IRAM loop time: %lld us\n", end - start);
}

void app_main(void) {
  /*  ========== RTC vs DRAM ========== */
  // gpio_reset_pin(LED_GPIO);
  // gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  // printf("blink_led_task address: %p\n", blink_led_task);
  // xTaskCreate(blink_led_task, "blink_task", 2048, NULL, 5, NULL);
  
  /*  ========== DMA vs DRAM ========== */
  // i2s_dma_example();

  /*  ========== IRAM vs Flash ========== */
  //cache_timing_experiment();
}
