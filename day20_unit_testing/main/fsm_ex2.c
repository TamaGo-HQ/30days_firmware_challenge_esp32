#include "fsm.h"

// ===== Handler functions (logic only) =====
static app_state_t init_to_off(app_state_t state) {
    return STATE_LED_OFF;
}

static app_state_t off_to_on(app_state_t state) {
    return STATE_LED_ON;
}

static app_state_t on_to_off(app_state_t state) {
    return STATE_LED_OFF;
}

static app_state_t stay(app_state_t state) {
    return state;
}

// ===== State/Event table =====
typedef app_state_t (*state_handler_t)(app_state_t);

static state_handler_t fsm_table[NUM_STATES][NUM_EVENTS] = {
    [STATE_INIT] = {
        [EVENT_INIT_DONE]   = init_to_off,
        [EVENT_BUTTON_PRESS] = stay
    },
    [STATE_LED_OFF] = {
        [EVENT_INIT_DONE]   = stay,
        [EVENT_BUTTON_PRESS] = off_to_on
    },
    [STATE_LED_ON] = {
        [EVENT_INIT_DONE]   = stay,
        [EVENT_BUTTON_PRESS] = on_to_off
    }
};

app_state_t fsm_dispatch(app_state_t current_state, app_event_t event)
{
    state_handler_t handler = fsm_table[current_state][event];
    if (handler) {
        return handler(current_state);
    }
    return current_state;
}
