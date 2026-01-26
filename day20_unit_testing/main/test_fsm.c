#include "unity.h"
#include "fsm.h"

// OFF + BUTTON_PRESS → ON
void test_led_off_button_press_goes_to_on(void)
{
    app_state_t next =
        fsm_dispatch(STATE_LED_OFF, EVENT_BUTTON_PRESS);

    TEST_ASSERT_EQUAL(STATE_LED_ON, next);
}

// ON + BUTTON_PRESS → OFF
void test_led_on_button_press_goes_to_off(void)
{
    app_state_t next =
        fsm_dispatch(STATE_LED_ON, EVENT_BUTTON_PRESS);

    TEST_ASSERT_EQUAL(STATE_LED_OFF, next);
}

// INIT + INIT_DONE → LED_OFF
void test_init_done_goes_to_led_off(void)
{
    app_state_t next =
        fsm_dispatch(STATE_INIT, EVENT_INIT_DONE);

    TEST_ASSERT_EQUAL(STATE_LED_OFF, next);
}

void app_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_led_off_button_press_goes_to_on);
    RUN_TEST(test_led_on_button_press_goes_to_off);
    RUN_TEST(test_init_done_goes_to_led_off);

    UNITY_END();
}
