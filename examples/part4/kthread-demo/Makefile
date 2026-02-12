# SPDX-License-Identifier: GPL-2.0
#
# Kthread Demo Makefile

obj-m := kthread_demo.o

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
	sudo insmod kthread_demo.ko

# Unload module
unload:
	sudo rmmod kthread_demo

# Reload (unload + load)
reload: unload load

# Quick test
test: load
	@echo "=== Kthread Demo Test ==="
	@sleep 3
	@cat /proc/kthread_demo
	@echo "event" | sudo tee /proc/kthread_demo
	@echo "event" | sudo tee /proc/kthread_demo
	@echo "event" | sudo tee /proc/kthread_demo
	@sleep 1
	@cat /proc/kthread_demo
	@echo "=== Test Complete ==="

.PHONY: all modules clean load unload reload test
