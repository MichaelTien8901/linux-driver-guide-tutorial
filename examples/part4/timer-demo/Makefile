# SPDX-License-Identifier: GPL-2.0
#
# Timer Demo Makefile

obj-m := timer_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Default target
all: modules

# Build kernel module
modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# Clean build artifacts
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

# Load module
load:
	sudo insmod timer_demo.ko

# Unload module
unload:
	sudo rmmod timer_demo

# Reload (unload + load)
reload: unload load

# Quick test
test: load
	@echo "=== Timer Demo Test ==="
	@cat /proc/timer_demo
	@echo "start" | sudo tee /proc/timer_demo
	@echo "hstart" | sudo tee /proc/timer_demo
	@sleep 5
	@cat /proc/timer_demo
	@echo "stop" | sudo tee /proc/timer_demo
	@echo "hstop" | sudo tee /proc/timer_demo
	@cat /proc/timer_demo
	@echo "=== Test Complete ==="

.PHONY: all modules clean load unload reload test
