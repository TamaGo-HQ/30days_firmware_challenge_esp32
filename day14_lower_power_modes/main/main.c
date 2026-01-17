/**
 * @file main.c day14_low_power_modes
*/

#include <stdio.h>
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_pm.h"

#define WAKEUP_GPIO GPIO_NUM_4   // Change if needed

static const char *TAG = "Wake_TEST";

// global variable in normal RAM
int ram_counter = 0;


// RTC SLOW memory variable (will survive deep sleep) 
RTC_DATA_ATTR int rtc_counter = 0;

// for exercice 3
RTC_DATA_ATTR int wake_count = 0;

RTC_DATA_ATTR int loop_counter = 0;

void print_task(void *arg)
{
    while (1) {
        ram_counter++;
        ESP_LOGI(TAG, "Loop counter: %d", ram_counter);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second
    }
}

void idle_task(void *arg)
{
    int count = 0;
    while(1) {
        ESP_LOGI("TASK", "Going idle, iteration %d", count++);
        // Block for 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void configure_pm()
{
    // New struct in ESP-IDF v5.x
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 160,       // max CPU frequency in MHz
        .min_freq_mhz = 80,        // min CPU frequency in MHz
        .light_sleep_enable = true // enable auto light sleep
    };
    
    esp_err_t err = esp_pm_configure(&pm_config);
    if (err != ESP_OK) {
        ESP_LOGE("PM", "Failed to configure PM: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI("PM", "Power management configured successfully");
    }
}


void app_main(void)
{
    /*  ========== Exercice 1 ========== */
    // // get wake up cuase
    // ESP_LOGI(TAG, "Wakeup cause: %d", esp_sleep_get_wakeup_cause());

    // // increment counter
    // ram_counter++;

    // ESP_LOGI(TAG, "RAM Counter = %d", ram_counter);

    // // configure timer wakepup : 5 secs
    // esp_sleep_enable_timer_wakeup(5 * 1000000ULL);

    // ESP_LOGI(TAG, "Going to deep sleep for 5 seconds...");

    // // enter deep sleep
    // esp_deep_sleep_start();

    /*  ========== Exercice 2 ========== */
    // esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    // ESP_LOGI(TAG, "Wakeup cause: %d", cause);

    // if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
    // ESP_LOGI(TAG, "Cold boot detected, resetting RTC counter");
    // rtc_counter = 0;
    // }

    // /* Increment both counters */
    // ram_counter++;
    // rtc_counter++;

    // ESP_LOGI(TAG, "RAM counter = %d", ram_counter);
    // ESP_LOGI(TAG, "RTC counter    = %d", rtc_counter);

    // /* Wake up every 5 seconds */
    // esp_sleep_enable_timer_wakeup(5 * 1000000ULL);

    // ESP_LOGI(TAG, "Entering deep sleep...\n");

    // esp_deep_sleep_start();

    /*  ========== exercice 3 ========== */
    // esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    // wake_count++;

    // ESP_LOGI(TAG, "Wake count: %d", wake_count);

    // switch (cause) {
    //     case ESP_SLEEP_WAKEUP_TIMER:
    //         ESP_LOGI(TAG, "Wakeup cause: TIMER");
    //         break;

    //     case ESP_SLEEP_WAKEUP_EXT0:
    //         ESP_LOGI(TAG, "Wakeup cause: BUTTON (EXT0)");
    //         break;

    //     case ESP_SLEEP_WAKEUP_UNDEFINED:
    //         ESP_LOGI(TAG, "Wakeup cause: COLD BOOT");
    //         break;

    //     default:
    //         ESP_LOGI(TAG, "Wakeup cause: OTHER (%d)", cause);
    //         break;
    // }
    // //Configure button GPIO as RTC GPIO 
    // rtc_gpio_init(WAKEUP_GPIO);
    // rtc_gpio_set_direction(WAKEUP_GPIO, RTC_GPIO_MODE_INPUT_ONLY);
    // rtc_gpio_pullup_en(WAKEUP_GPIO);    // Button to GND
    // rtc_gpio_pulldown_dis(WAKEUP_GPIO);

    // // Wake up when button is pressed (GPIO goes LOW) 
    // esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);

    // // Timer wake-up after 10 seconds 
    // esp_sleep_enable_timer_wakeup(10 * 1000000ULL);

    // ESP_LOGI(TAG, "Going to deep sleep...");
    // ESP_LOGI(TAG, "Press the button OR wait 10 seconds\n");

    // esp_deep_sleep_start();
    
    /*  ========== exercice 4 ========== */

    // esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    // ESP_LOGI(TAG, "Wakeup cause: %d", cause);

    // /* Create a task that prints every second */
    // xTaskCreate(print_task, "print_task", 2048, NULL, 5, NULL);

    // /* --- LIGHT SLEEP SETUP --- */
    // rtc_gpio_init(WAKEUP_GPIO);
    // rtc_gpio_set_direction(WAKEUP_GPIO, RTC_GPIO_MODE_INPUT_ONLY);
    // rtc_gpio_pullup_en(WAKEUP_GPIO);    // Button to GND
    // rtc_gpio_pulldown_dis(WAKEUP_GPIO);

    // /* Wakeup sources */
    // esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);          // Button
    // esp_sleep_enable_timer_wakeup(10 * 1000000ULL);         // 10 seconds

    // ESP_LOGI(TAG, "Going to LIGHT sleep for 10s or button press...\n");
    // //vTaskDelay(pdMS_TO_TICKS(100));
    // //uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(500));


    // // Light sleep — CPU pauses but context preserved 
    // esp_light_sleep_start();

    // ESP_LOGI(TAG, "Woke up from LIGHT sleep!");
    // // Task resumes automatically where it left off 
    /*  ========== exercice 5 ========== */
    ESP_LOGI("MAIN", "Starting Tickless Idle Example");

    // Configure power management
    configure_pm();

    // Create blocking task
    xTaskCreate(idle_task, "idle_task", 2048, NULL, 5, NULL);

    ESP_LOGI("MAIN", "Tasks created. System will automatically sleep when idle");

}
