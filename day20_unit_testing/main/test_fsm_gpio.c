#include "unity.h"
#include "fsm.h"
#define LED_GPIO 4  // or any number; we just need a consistent value for testing

// ---------------------
// Mock GPIO
// ---------------------
static int last_level = -1;

void gpio_write(int pin, int level) {
    last_level = level;
}

// ---------------------
// Tests
// ---------------------

void test_init_to_off_sets_led_low(void) {
    last_level = -1;

    app_state_t next = fsm_dispatch(STATE_INIT, EVENT_INIT_DONE);

    TEST_ASSERT_EQUAL(STATE_LED_OFF, next);
    TEST_ASSERT_EQUAL(0, last_level);
}

void test_off_to_on_sets_led_high(void) {
    last_level = -1;

    app_state_t next = fsm_dispatch(STATE_LED_OFF, EVENT_BUTTON_PRESS);

    TEST_ASSERT_EQUAL(STATE_LED_ON, next);
    TEST_ASSERT_EQUAL(1, last_level);
}

void test_on_to_off_sets_led_low(void) {
    last_level = -1;

    app_state_t next = fsm_dispatch(STATE_LED_ON, EVENT_BUTTON_PRESS);

    TEST_ASSERT_EQUAL(STATE_LED_OFF, next);
    TEST_ASSERT_EQUAL(0, last_level);
}

void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_init_to_off_sets_led_low);
    RUN_TEST(test_off_to_on_sets_led_high);
    RUN_TEST(test_on_to_off_sets_led_low);

    UNITY_END();
}
