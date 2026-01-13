#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mathlib.h"
#include <math.h>
#include <stdio.h>

// extern void dummy_function(void);

// Define dummy computation functions
void compute1(void) {
  volatile double x = 0;
  for (int i = 0; i < 1000; i++) {
    x += sqrt(i) * sin(i);
  }
}

void compute2(void) {
  volatile double y = 0;
  for (int i = 0; i < 1000; i++) {
    y += log(i + 1) * cos(i);
  }
}

void compute3(void) {
  volatile double z = 0;
  for (int i = 0; i < 1000; i++) {
    z += tan(i) / (i + 1);
  }
}

void compute4(void) {
  volatile double a = 0;
  for (int i = 0; i < 1000; i++) {
    a += sqrt(i);
  }
}
void compute5(void) {
  volatile double b = 0;
  for (int i = 0; i < 1000; i++) {
    b += log(i + 1);
  }
}
void compute6(void) {
  volatile double c = 0;
  for (int i = 0; i < 1000; i++) {
    c += sin(i);
  }
}
void compute7(void) {
  volatile double d = 0;
  for (int i = 0; i < 1000; i++) {
    d += cos(i);
  }
}
void compute8(void) {
  volatile double e = 0;
  for (int i = 0; i < 1000; i++) {
    e += tan(i);
  }
}

void run_computes(void) {
  compute1();
  compute2();
  compute3();
  compute4();
  compute5();
  compute6();
  compute7();
  compute8();
}

void app_main(void) {
  /*  ========== default vs -0s compilation flag========== */
  //   dummy_function();

  /*  ========== optimization for size vs speed ========== */
  printf("Starting optimization test...\n");

  // Measure start time
  int64_t start_time = esp_timer_get_time();

  // Run compute functions multiple times
  for (int i = 0; i < 10; i++) {
    run_computes();
  }

  // Measure end time
  int64_t end_time = esp_timer_get_time();
  int64_t elapsed_us = end_time - start_time;

  printf("Done!\n");
  printf("Elapsed time: %lld microseconds (~%.2f ms)\n", elapsed_us,
         elapsed_us / 1000.0);

  /*  ========== Components ========== */

  int x = add(2, 3);
  int y = multiply(4, 5);

  printf("add(2,3) = %d\n", x);
  printf("multiply(4,5) = %d\n", y);
}
