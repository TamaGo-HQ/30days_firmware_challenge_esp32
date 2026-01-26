#include "unity.h"
#include "math_utils.h"

void test_clamp_inside_range(void)
{
    int result = clamp(5, 0, 10);
    TEST_ASSERT_EQUAL(5, result);
}

void test_clamp_below_min(void)
{
    int result = clamp(-3, 0, 10);
    TEST_ASSERT_EQUAL(0, result);
}

void test_clamp_above_max(void)
{
    int result = clamp(15, 0, 10);
    TEST_ASSERT_EQUAL(10, result);
}

void app_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_clamp_inside_range);
    RUN_TEST(test_clamp_below_min);
    RUN_TEST(test_clamp_above_max);

    UNITY_END();
}
