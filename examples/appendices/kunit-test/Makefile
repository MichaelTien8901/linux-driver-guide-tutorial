# SPDX-License-Identifier: GPL-2.0
obj-m := kunit_example_test.o
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KDIR ?= $(KERNEL_DIR)
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Note: KUnit tests are best run with kunit.py tool
# This Makefile allows building as module for demonstration

.PHONY: all clean
