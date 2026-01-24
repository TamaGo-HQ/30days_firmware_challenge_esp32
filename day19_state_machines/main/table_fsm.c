#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// GPIO definitions
#define LED_GPIO     GPIO_NUM_4
#define BUTTON_GPIO  GPIO_NUM_12

// FSM states and events
typedef enum {
    STATE_INIT,
    STATE_LED_OFF,
    STATE_LED_ON,
    NUM_STATES
} app_state_t;

typedef enum {
    EVENT_INIT_DONE,
    EVENT_BUTTON_PRESS,
    NUM_EVENTS
} app_event_t;

// Event queue
static QueueHandle_t event_queue;

// ===== Handler functions =====
// Each handler receives current state and returns new state
app_state_t init_to_off(app_state_t state) {
    gpio_set_level(LED_GPIO, 0);
    return STATE_LED_OFF;
}

app_state_t off_to_on(app_state_t state) {
    gpio_set_level(LED_GPIO, 1);
    return STATE_LED_ON;
}

app_state_t on_to_off(app_state_t state) {
    gpio_set_level(LED_GPIO, 0);
    return STATE_LED_OFF;
}

app_state_t stay(app_state_t state) {
    return state;
}

// ===== State/Event table =====
typedef app_state_t (*state_handler_t)(app_state_t);

state_handler_t fsm_table[NUM_STATES][NUM_EVENTS] = {
    [STATE_INIT] = {
        [EVENT_INIT_DONE] = init_to_off,
        [EVENT_BUTTON_PRESS] = stay
    },
    [STATE_LED_OFF] = {
        [EVENT_INIT_DONE] = stay,
        [EVENT_BUTTON_PRESS] = off_to_on
    },
    [STATE_LED_ON] = {
        [EVENT_INIT_DONE] = stay,
        [EVENT_BUTTON_PRESS] = on_to_off
    }
};

// ===== FSM dispatcher (functional) =====
static app_state_t fsm_dispatch(app_state_t current_state, app_event_t event) {
    state_handler_t handler = fsm_table[current_state][event];
    if (handler) {
        return handler(current_state);
    }
    return current_state;
}

// ===== GPIO initialization =====
static void gpio_init(void)
{
    // LED
    gpio_config_t led_cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_cfg);

    // Button (active-low, falling edge)
    gpio_config_t btn_cfg = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&btn_cfg);

    // Install ISR service
    gpio_install_isr_service(0);
}

// ===== Button ISR with debounce =====
static void IRAM_ATTR button_isr_handler(void* arg)
{
    static TickType_t last_tick = 0;
    TickType_t now = xTaskGetTickCountFromISR();

    if ((now - last_tick) > pdMS_TO_TICKS(50)) {  // 50 ms debounce
        app_event_t evt = EVENT_BUTTON_PRESS;
        xQueueSendFromISR(event_queue, &evt, NULL);
        last_tick = now;
    }
}

// ===== FSM task =====
void fsm_task(void *arg)
{
    app_state_t state = STATE_INIT;

    // Initial FSM transition
    state = fsm_dispatch(state, EVENT_INIT_DONE);

    app_event_t event;
    while (1) {
        if (xQueueReceive(event_queue, &event, portMAX_DELAY)) {
            state = fsm_dispatch(state, event);
        }
    }
}

// ===== Main =====
void app_main(void)
{
    // Create event queue
    event_queue = xQueueCreate(10, sizeof(app_event_t));

    gpio_init();

    // Attach button ISR
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    // Create FSM task
    xTaskCreate(
        fsm_task,
        "fsm_task",
        2048,
        NULL,
        5,
        NULL
    );
}
