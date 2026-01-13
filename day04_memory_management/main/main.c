/**
 * @file main.c
 * @brief Experiments with heap and stack and static memory
 */

#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "MEM_EXP";

/* ------------------------------------------------------------ */
/*  Static allocation                                         */
/* ------------------------------------------------------------ */
static uint8_t static_buf[512];
/* ------------------------------------------------------------ */
/*  Stack allocation inside a task                             */
/* ------------------------------------------------------------ */
// Task stack and TCB (static allocation)
#define STACK_SIZE 2048
static StackType_t stack_mem[STACK_SIZE];
static StaticTask_t tcb_mem;

static void stack_task(void *arg) {
  ESP_LOGI(TAG, "Stack task started");

  // Task stack addresses
  ESP_LOGI(TAG, "stack_mem start @ %p", (void *)stack_mem);
  ESP_LOGI(TAG, "stack_mem end   @ %p",
           (void *)((uint8_t *)stack_mem + STACK_SIZE));

  uint8_t stack_buf[128];
  ESP_LOGI(TAG, "stack_buf @ %p", (void *)stack_buf);

  if ((void *)stack_buf >= (void *)stack_mem &&
      (void *)stack_buf <= (void *)((uint8_t *)stack_mem + STACK_SIZE)) {
    ESP_LOGI(TAG, "stack_buf is inside the task stack region ✅");
  } else {
    ESP_LOGW(TAG, "stack_buf is NOT inside the task stack region ❌");
  }

  UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
  ESP_LOGI(TAG, "Stack task high water mark: %u bytes", high_water);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/* ------------------------------------------------------------ */
/* Helper: Print heap info                                       */
/* ------------------------------------------------------------ */
static void print_heap(const char *label) {
  ESP_LOGI(TAG, "===== HEAP INFO: %s =====", label);

  size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

  ESP_LOGI(TAG, "Free heap        : %u bytes", free_heap);
  ESP_LOGI(TAG, "Largest free blk : %u bytes", largest_block);

  /* Detailed per-region info */
  heap_caps_print_heap_info(MALLOC_CAP_8BIT);

  ESP_LOGI(TAG, "====================================");
}

/* ------------------------------------------------------------ */
/* Dummy task to consume stack                                  */
/* ------------------------------------------------------------ */
static void dummy_task(void *arg) {
  /* Allocate something on the stack */
  uint8_t stack_buffer[256];
  (void)stack_buffer;

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void task_fn(void *arg) {
  uint8_t big_buf[2000];
  //   for (int i = 0; i < 2000; i++)
  //     big_buf[i] = i; // touch memory to force usage
  //   UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
  //   ESP_LOGI(TAG, "Stack task high water mark: %u bytes", high_water);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(10)); // yield so FreeRTOS can check canary
  }
}
// Memory pool configuration
#define POOL_SIZE 10  // number of blocks
#define BLOCK_SIZE 64 // size of each block

static uint8_t pool[POOL_SIZE][BLOCK_SIZE];
static bool used[POOL_SIZE] = {0}; // false = free

// Allocate a block from the pool
void *pool_alloc(void) {
  for (int i = 0; i < POOL_SIZE; i++) {
    if (!used[i]) {
      used[i] = true;
      ESP_LOGI(TAG, "Allocated block %d at %p", i, pool[i]);
      return pool[i];
    }
  }
  ESP_LOGW(TAG, "Memory pool exhausted!");
  return NULL; // No free blocks
}

// Free a block back to the pool
void pool_free(void *ptr) {
  for (int i = 0; i < POOL_SIZE; i++) {
    if (ptr == pool[i]) {
      used[i] = false;
      ESP_LOGI(TAG, "Freed block %d at %p", i, pool[i]);
      return;
    }
  }
  ESP_LOGW(TAG, "Attempt to free invalid pointer: %p", ptr);
}

// Example task to test memory pool
void uart_sim_task(void *arg) {
  int msg_count = 0;

  while (1) {
    // Simulate receiving a message
    void *buf = pool_alloc();
    if (buf != NULL) {
      // Fill buffer with dummy message
      snprintf((char *)buf, BLOCK_SIZE, "Message #%d", msg_count++);
      ESP_LOGI(TAG, "Received: %s", buf);

      // Simulate processing delay
      vTaskDelay(pdMS_TO_TICKS(200));

      // Free buffer
      // pool_free(buf);
    } else {
      ESP_LOGW(TAG, "Pool exhausted! Cannot receive new message.");
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}

/* ------------------------------------------------------------ */
/* Main entry point                                             */
/* ------------------------------------------------------------ */
void app_main(void) {

  ESP_LOGI(TAG, "Booting...");
  print_heap("After boot");

  //   /*  ========== 1 Dummy Task Creation ========== */
  //   xTaskCreate(dummy_task, "dummy_task",
  //               2048, // stack size in bytes
  //               NULL, 5, NULL);
  //   print_heap("After task creation");


  //   /*  ========== 2 Dynamic Variable Declaration ========== */
  //   void *p1 = malloc(1024);
  //   void *p2 = malloc(2048);
  //   ESP_LOGI(TAG, "Allocated p1=1024 bytes, p2=2048 bytes");
  //   //   ESP_LOGI(TAG, "p1=%p p2=%p", p1, p2);
  //   print_heap("After malloc");


  /*  ========== 3 Static vs Heap vs Stack ========== */
  //   /*  Static memory — already allocated at compile time */
  //   ESP_LOGI(TAG, "Static buffer declared (512 bytes)");
  //   ESP_LOGI(TAG, "static_buf @ =%p ", static_buf);
  //   print_heap("After static allocation");

  //   /*  Stack allocation — create a task */
  //   xTaskCreateStatic(stack_task, "stack_task", STACK_SIZE, NULL, 5,
  //   stack_mem,
  //                     &tcb_mem);
  //   vTaskDelay(pdMS_TO_TICKS(500));
  //   print_heap("After stack task creation");

  //   /*  Heap allocation */
  //   uint8_t *heap_buf = heap_caps_malloc(512, MALLOC_CAP_8BIT);
  //   for (int i = 0; i < 512; i++)
  //     heap_buf[i] = i % 256; // Use it
  //   ESP_LOGI(TAG, "Heap buffer allocated (512 bytes) at %p", heap_buf);
  //   print_heap("After heap allocation");


  /*  ========== 4 Deliberate Stack Overflow ========== */
  //   print_heap("Before Stack Overflow");
  //   xTaskCreate(task_fn, "bad_task", 1024, NULL, 5, NULL);
  //   print_heap("After Stack Overflow");


  /*  ========== 5 Memory Pool ========== */
  ESP_LOGI(TAG, "Starting Memory Pool Experiment");
  print_heap("Before UART Simulation");
  xTaskCreate(uart_sim_task, "uart_task", 2048, NULL, 5, NULL);
  print_heap("After UART Simulation");
}
