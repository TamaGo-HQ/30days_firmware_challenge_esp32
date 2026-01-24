#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// GPIO definitions
#define LED_GPIO     GPIO_NUM_4      // External LED
#define BUTTON_GPIO  GPIO_NUM_12     // Example button

// FSM types
typedef enum {
    STATE_INIT,
    STATE_LED_OFF,
    STATE_LED_ON
} app_state_t;

typedef enum {
    EVENT_INIT_DONE,
    EVENT_BUTTON_PRESSED
} app_event_t;

// Queue for passing events from ISR to FSM task
static QueueHandle_t event_queue;

// ===== FSM function (functional style) =====
app_state_t fsm_handle_event(app_state_t current_state, app_event_t event)
{
    switch (current_state) {

        case STATE_INIT:
            if (event == EVENT_INIT_DONE) {
                gpio_set_level(LED_GPIO, 0);   // entry action
                return STATE_LED_OFF;
            }
            break;

        case STATE_LED_OFF:
            if (event == EVENT_BUTTON_PRESSED) {
                gpio_set_level(LED_GPIO, 1);   // entry action
                return STATE_LED_ON;
            }
            break;

        case STATE_LED_ON:
            if (event == EVENT_BUTTON_PRESSED) {
                gpio_set_level(LED_GPIO, 0);   // entry action
                return STATE_LED_OFF;
            }
            break;

        default:
            return STATE_INIT;
    }

    // If no transition, return the same state
    return current_state;
}

// ===== Button ISR =====
static void IRAM_ATTR button_isr_handler(void* arg)
{
    app_event_t evt = EVENT_BUTTON_PRESSED;
    xQueueSendFromISR(event_queue, &evt, NULL);
}

// ===== GPIO initialization =====
static void gpio_init(void)
{
    // LED GPIO
    gpio_config_t led_cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_cfg);

    // Button GPIO (active-low, internal pull-up, falling edge interrupt)
    gpio_config_t btn_cfg = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&btn_cfg);

    // Install ISR service and attach handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);
}

// ===== FSM task =====
void fsm_task(void *arg)
{
    app_state_t state = STATE_INIT;

    // Initial FSM transition
    state = fsm_handle_event(state, EVENT_INIT_DONE);

    app_event_t event;

    while (1) {
        if (xQueueReceive(event_queue, &event, portMAX_DELAY)) {
            state = fsm_handle_event(state, event);
        }
    }
}

// ===== Main =====
void app_main(void)
{
    // Create event queue
    event_queue = xQueueCreate(10, sizeof(app_event_t));

    // Initialize GPIOs
    gpio_init();

    // Create FSM task
    xTaskCreate(
        fsm_task,       // task function
        "fsm_task",     // name
        2048,           // stack size
        NULL,           // parameters
        5,              // priority
        NULL            // task handle
    );
}
