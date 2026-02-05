# SPDX-License-Identifier: GPL-2.0
#
# Makefile for LED Demo Driver
#

obj-m := led_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod led_demo.ko

unload:
	sudo rmmod led_demo

test:
	@echo "=== LED Demo Test ==="
	@echo ""
	@echo "Registered LEDs:"
	@for led in /sys/class/leds/demo:*; do \
		if [ -d "$$led" ]; then \
			name=$$(basename $$led); \
			brightness=$$(cat $$led/brightness); \
			max=$$(cat $$led/max_brightness); \
			trigger=$$(cat $$led/trigger | grep -o '\[[^]]*\]' | tr -d '[]'); \
			echo "  $$name: brightness=$$brightness/$$max trigger=$$trigger"; \
		fi; \
	done
	@echo ""
	@echo "Available triggers:"
	@cat /sys/class/leds/demo:red:status/trigger 2>/dev/null | tr ' ' '\n' | head -10
	@echo "  ..."

.PHONY: all clean load unload test
