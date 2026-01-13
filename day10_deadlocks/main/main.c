#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>

enum { NUM_TASKS = 5 };          // Number of tasks (philosophers)
enum { TASK_STACK_SIZE = 2048 }; // Bytes in ESP32, words in vanilla FreeRTOS

// Globals
static SemaphoreHandle_t bin_sem;  // Wait for parameters to be read
static SemaphoreHandle_t done_sem; // Notifies main task when done
static SemaphoreHandle_t chopstick[NUM_TASKS];
static const char *TAG = "DINNER";

TickType_t mutex_timeout = 5;
static SemaphoreHandle_t butler;

// The only task: eating
void eat(void *parameters) {

  int num;
  char buf[50];

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);

  // Take left chopstick
  xSemaphoreTake(chopstick[num], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num, num);
  printf(buf);

  // Add some delay to force deadlock
  vTaskDelay(1);

  // Take right chopstick
  xSemaphoreTake(chopstick[(num + 1) % NUM_TASKS], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num,
          (num + 1) % NUM_TASKS);
  printf(buf);

  // Do some eating
  sprintf(buf, "Philosopher %i is eating\n", num);
  printf(buf);
  vTaskDelay(10);

  // Put down right chopstick
  xSemaphoreGive(chopstick[(num + 1) % NUM_TASKS]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num,
          (num + 1) % NUM_TASKS);
  printf(buf);

  // Put down left chopstick
  xSemaphoreGive(chopstick[num]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num, num);
  printf(buf);

  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}

void eat_timeout(void *parameters) {

  int num;
  char buf[50];

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);
  while (1) {
    // Take left chopstick
    if (xSemaphoreTake(chopstick[num], mutex_timeout) == pdTRUE) {

      sprintf(buf, "Philosopher %i took chopstick %i\n", num, num);
      printf(buf);

      // Add some delay to force deadlock
      vTaskDelay(1);

      // Take right chopstick
      if (xSemaphoreTake(chopstick[(num + 1) % NUM_TASKS], mutex_timeout) ==
          pdTRUE) {
        sprintf(buf, "Philosopher %i took chopstick %i\n", num,
                (num + 1) % NUM_TASKS);
        printf(buf);

        // Do some eating
        sprintf(buf, "Philosopher %i is eating\n", num);
        printf(buf);
        vTaskDelay(10);

        // Put down right chopstick
        xSemaphoreGive(chopstick[(num + 1) % NUM_TASKS]);
        sprintf(buf, "Philosopher %i returned chopstick %i\n", num,
                (num + 1) % NUM_TASKS);
        printf(buf);

        // Put down left chopstick
        xSemaphoreGive(chopstick[num]);
        sprintf(buf, "Philosopher %i returned chopstick %i\n", num, num);
        printf(buf);

        // Notify main task and delete self
        xSemaphoreGive(done_sem);
        vTaskDelete(NULL);
      } else {
        sprintf(buf, "Philosopher %i timed out waiting for chopstick %i\n", num,
                (num + 1) % NUM_TASKS);
        printf(buf);
        xSemaphoreGive(chopstick[num]);
        sprintf(buf, "Philosopher %i returned chopstick %i\n", num, num);
        printf(buf);
        // let the next one pick it up
        vTaskDelay(1);
      }
    } else {
      sprintf(buf, "Philosopher %i timed out waiting for chopstick %i\n", num,
              num);
      printf(buf);
    }
  }
}

void eat_lowest(void *parameters) {

  int num;
  char buf[50];

  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);

  int left = num;
  int right = (num + 1) % NUM_TASKS;

  int first = (left < right) ? left : right;
  int second = (left < right) ? right : left;

  // Take lowest-numbered chopstick first
  xSemaphoreTake(chopstick[first], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num, first);
  printf("%s", buf);

  vTaskDelay(1);

  // Take highest-numbered chopstick second
  xSemaphoreTake(chopstick[second], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num, second);
  printf("%s", buf);

  // Eat
  sprintf(buf, "Philosopher %i is eating\n", num);
  printf("%s", buf);
  vTaskDelay(10);

  // Release in reverse order (good practice)
  xSemaphoreGive(chopstick[second]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num, second);
  printf("%s", buf);

  xSemaphoreGive(chopstick[first]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num, first);
  printf("%s", buf);

  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}

void eat_butler(void *parameters) {

  int num;
  char buf[50];

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);

  // Ask the butler for permission to eat
  xSemaphoreTake(butler, portMAX_DELAY);
  sprintf(buf, "Philosopher %i got permission from the butler\n", num);
  printf("%s", buf);

  // Take left chopstick
  xSemaphoreTake(chopstick[num], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num, num);
  printf(buf);

  // Add some delay to force deadlock
  vTaskDelay(1);

  // Take right chopstick
  xSemaphoreTake(chopstick[(num + 1) % NUM_TASKS], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num,
          (num + 1) % NUM_TASKS);
  printf(buf);

  // Do some eating
  sprintf(buf, "Philosopher %i is eating\n", num);
  printf(buf);
  vTaskDelay(10);

  // Put down right chopstick
  xSemaphoreGive(chopstick[(num + 1) % NUM_TASKS]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num,
          (num + 1) % NUM_TASKS);
  printf(buf);

  // Put down left chopstick
  xSemaphoreGive(chopstick[num]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num, num);
  printf(buf);

  // Release the butler
  xSemaphoreGive(butler);
  sprintf(buf, "Philosopher %i released the butler\n", num);
  printf("%s", buf);

  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}

void app_main(void) {
  char task_name[32];

  printf("---FreeRTOS Dining Philosophers Challenge---\n");

  // Create kernel objects before starting tasks
  bin_sem = xSemaphoreCreateBinary();
  done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);
  for (int i = 0; i < NUM_TASKS; i++) {
    chopstick[i] = xSemaphoreCreateMutex();
  }

  butler = xSemaphoreCreateBinary();
  xSemaphoreGive(butler); // Butler is initially available

  ESP_LOGI(TAG, "Eating Started at @ tick %lu", xTaskGetTickCount());
  // Have the philosphers start eating
  for (int i = 0; i < NUM_TASKS; i++) {
    sprintf(task_name, "Philosopher %i\n", i);
    xTaskCreatePinnedToCore(eat_butler, task_name, TASK_STACK_SIZE, (void *)&i,
                            1, NULL, 0);
    xSemaphoreTake(bin_sem, portMAX_DELAY);
  }

  // Wait until all the philosophers are done
  for (int i = 0; i < NUM_TASKS; i++) {
    xSemaphoreTake(done_sem, portMAX_DELAY);
  }
  ESP_LOGI(TAG, "Eating Done at @ tick %lu", xTaskGetTickCount());
  printf("Done! No deadlock occurred!\n");
}
