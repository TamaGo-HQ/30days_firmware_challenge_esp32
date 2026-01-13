#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "MEM_MON";

static void render_bar(char *buf, size_t buf_len, int percent) {
  const int BAR_WIDTH = 10;
  int filled = (percent * BAR_WIDTH) / 100;

  int idx = 0;
  buf[idx++] = '[';

  for (int i = 0; i < BAR_WIDTH; i++) {
    buf[idx++] = (i < filled) ? '■' : '_';
  }

  buf[idx++] = ']';
  buf[idx] = '\0';
}

void mem_monitor_task(void *pvParameters) {
  ESP_LOGI(TAG, "Memory monitor started");

  const size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);

  char bar[16];

  while (1) {
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t min_free = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    int free_pct = (free_heap * 100) / total_heap;
    int min_pct = (min_free * 100) / total_heap;

    ESP_LOGI(TAG, "Heap status:");

    render_bar(bar, sizeof(bar), free_pct);
    ESP_LOGI(TAG, "  Free heap       %s %d%%", bar, free_pct);

    render_bar(bar, sizeof(bar), min_pct);
    ESP_LOGI(TAG, "  Min free (ever) %s %d%%", bar, min_pct);

    render_bar(bar, sizeof(bar), 0); // visual only
    ESP_LOGI(TAG, "  Largest block   %s %u KB", bar,
             (unsigned)(largest / 1024));

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void fragment_task(void *pvParameters) {
  void *blocks[5];

  while (1) {
    blocks[0] = malloc(1024);
    blocks[1] = malloc(2048);
    blocks[2] = malloc(4096);
    blocks[3] = malloc(8192);
    blocks[4] = malloc(16384);

    vTaskDelay(pdMS_TO_TICKS(2000));

    free(blocks[1]);
    free(blocks[3]);

    vTaskDelay(pdMS_TO_TICKS(2000));

    free(blocks[0]);
    free(blocks[2]);
    free(blocks[4]);

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void alloc_task(void *pvParameters) {
  const size_t alloc_size = 1024 * 10; // 10 KB
  void *ptr = NULL;

  while (1) {
    ptr = malloc(alloc_size);

    if (ptr != NULL) {
      ESP_LOGI("ALLOC", "Allocated %u bytes", alloc_size);
      vTaskDelay(pdMS_TO_TICKS(3000));

      free(ptr);
      ESP_LOGI("ALLOC", "Freed %u bytes", alloc_size);
    } else {
      ESP_LOGE("ALLOC", "Allocation failed!");
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void app_main(void) {
  xTaskCreate(mem_monitor_task, "memory_monitor", 2048, NULL, 5, NULL);
  xTaskCreate(alloc_task, "alloc_task", 2048, NULL, 5, NULL);
  xTaskCreate(fragment_task, "frag_task", 2048, NULL, 5, NULL);
}
