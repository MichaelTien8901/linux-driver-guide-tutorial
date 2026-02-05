// SPDX-License-Identifier: GPL-2.0
/*
 * KUnit Test Example
 *
 * Demonstrates basic KUnit test structure.
 */

#include <kunit/test.h>

/* Function to test (normally in separate file) */
static int example_add(int a, int b)
{
    return a + b;
}

static int example_clamp(int val, int min, int max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

/* Test cases */
static void test_add_positive(struct kunit *test)
{
    KUNIT_EXPECT_EQ(test, example_add(2, 3), 5);
    KUNIT_EXPECT_EQ(test, example_add(0, 0), 0);
}

static void test_add_negative(struct kunit *test)
{
    KUNIT_EXPECT_EQ(test, example_add(-1, 1), 0);
    KUNIT_EXPECT_EQ(test, example_add(-5, -3), -8);
}

static void test_clamp(struct kunit *test)
{
    /* Within range */
    KUNIT_EXPECT_EQ(test, example_clamp(5, 0, 10), 5);

    /* Below min */
    KUNIT_EXPECT_EQ(test, example_clamp(-5, 0, 10), 0);

    /* Above max */
    KUNIT_EXPECT_EQ(test, example_clamp(15, 0, 10), 10);

    /* Edge cases */
    KUNIT_EXPECT_EQ(test, example_clamp(0, 0, 10), 0);
    KUNIT_EXPECT_EQ(test, example_clamp(10, 0, 10), 10);
}

/* Test suite */
static struct kunit_case example_test_cases[] = {
    KUNIT_CASE(test_add_positive),
    KUNIT_CASE(test_add_negative),
    KUNIT_CASE(test_clamp),
    {}
};

static struct kunit_suite example_test_suite = {
    .name = "example_tests",
    .test_cases = example_test_cases,
};
kunit_test_suite(example_test_suite);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KUnit Test Example");
