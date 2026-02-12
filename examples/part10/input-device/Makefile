# SPDX-License-Identifier: GPL-2.0
#
# Input Device Demo Makefile

obj-m := input_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: modules

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

load:
	sudo insmod input_demo.ko

unload:
	sudo rmmod input_demo

reload: unload load

test: load
	@echo "=== Input Device Demo Test ==="
	@cat /proc/input_demo
	@echo "click" | sudo tee /proc/input_demo
	@echo "click" | sudo tee /proc/input_demo
	@echo "power" | sudo tee /proc/input_demo
	@cat /proc/input_demo
	@echo ""
	@echo "Input devices:"
	@cat /proc/bus/input/devices | grep -A 4 "Virtual Button"
	@echo "=== Test Complete ==="
	@echo "Tip: run 'evtest' in another terminal to see events"

.PHONY: all modules clean load unload reload test
