#pragma once

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

app_state_t fsm_dispatch(app_state_t current_state, app_event_t event);
