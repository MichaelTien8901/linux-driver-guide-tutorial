# SPDX-License-Identifier: GPL-2.0
#
# Makefile for PWM Demo Driver
#

obj-m := pwm_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KDIR ?= $(KERNEL_DIR)
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod pwm_demo.ko

unload:
	sudo rmmod pwm_demo

test:
	@echo "=== PWM Demo Test ==="
	@ls /sys/class/pwm/ 2>/dev/null || echo "No PWM chips found"
	@echo ""
	@if [ -d /sys/class/pwm/pwmchip* ]; then \
		for chip in /sys/class/pwm/pwmchip*; do \
			echo "Found: $$chip"; \
			cat $$chip/npwm 2>/dev/null && echo " channels"; \
		done; \
	fi
	@echo ""
	@echo "Debugfs status:"
	@sudo cat /sys/kernel/debug/demo_pwm 2>/dev/null || echo "Debugfs not available"

.PHONY: all clean load unload test
