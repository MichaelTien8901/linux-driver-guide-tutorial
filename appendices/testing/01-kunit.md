---
layout: default
title: "E.1 KUnit"
parent: "Appendix E: Testing"
nav_order: 1
---

# KUnit

Unit testing framework for the kernel.

## Basic Test Structure

```c
#include <kunit/test.h>

/* Test case */
static void my_test_addition(struct kunit *test)
{
    int result = my_add(2, 3);
    KUNIT_EXPECT_EQ(test, result, 5);
}

static void my_test_null_check(struct kunit *test)
{
    void *ptr = my_alloc(0);
    KUNIT_EXPECT_NULL(test, ptr);
}

/* Test suite */
static struct kunit_case my_test_cases[] = {
    KUNIT_CASE(my_test_addition),
    KUNIT_CASE(my_test_null_check),
    {}
};

static struct kunit_suite my_test_suite = {
    .name = "my_driver_tests",
    .test_cases = my_test_cases,
};
kunit_test_suite(my_test_suite);

MODULE_LICENSE("GPL");
```

## Assertions

| Macro | Checks |
|-------|--------|
| `KUNIT_EXPECT_EQ(test, a, b)` | a == b |
| `KUNIT_EXPECT_NE(test, a, b)` | a != b |
| `KUNIT_EXPECT_LT(test, a, b)` | a < b |
| `KUNIT_EXPECT_TRUE(test, cond)` | condition true |
| `KUNIT_EXPECT_NULL(test, ptr)` | ptr is NULL |
| `KUNIT_EXPECT_NOT_ERR_OR_NULL(test, ptr)` | valid pointer |

`EXPECT` continues on failure; `ASSERT` stops test.

## Setup/Teardown

```c
static int my_test_init(struct kunit *test)
{
    struct my_context *ctx = kunit_kzalloc(test, sizeof(*ctx), GFP_KERNEL);
    test->priv = ctx;
    return 0;  /* 0 = success */
}

static void my_test_exit(struct kunit *test)
{
    /* kunit_kzalloc auto-freed */
}

static struct kunit_suite my_suite = {
    .name = "my_tests",
    .init = my_test_init,
    .exit = my_test_exit,
    .test_cases = my_test_cases,
};
```

## Running Tests

```bash
# Build with KUnit
./tools/testing/kunit/kunit.py run --kunitconfig=drivers/my_driver/.kunitconfig

# Or as module
insmod my_test.ko
# Results in dmesg

# Parse results
dmesg | grep -A 100 'my_driver_tests'
```

## Test Organization

```
drivers/my_driver/
├── my_driver.c
├── my_driver_test.c    # KUnit tests
├── Kconfig
├── Makefile
└── .kunitconfig        # KUnit config
```

`.kunitconfig`:
```
CONFIG_KUNIT=y
CONFIG_MY_DRIVER=y
CONFIG_MY_DRIVER_KUNIT_TEST=y
```

## Summary

1. Create test functions with `KUNIT_EXPECT_*`
2. Group in `kunit_case` array
3. Define `kunit_suite`
4. Register with `kunit_test_suite()`
5. Run with `kunit.py` or as module

## Further Reading

- [KUnit Documentation](https://docs.kernel.org/dev-tools/kunit/index.html)
- [Writing Tests](https://docs.kernel.org/dev-tools/kunit/usage.html)
