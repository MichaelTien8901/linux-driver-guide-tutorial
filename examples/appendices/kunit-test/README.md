# KUnit Test Example

Demonstrates KUnit test structure with KUNIT_EXPECT macros.

## Running (as module)

```bash
make
sudo insmod kunit_example_test.ko
dmesg | grep -A 20 'example_tests'
sudo rmmod kunit_example_test
```

## Running (with kunit.py)

```bash
# From kernel source
./tools/testing/kunit/kunit.py run example_tests
```

Results show pass/fail for each test case.
