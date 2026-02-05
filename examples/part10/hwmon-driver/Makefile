# SPDX-License-Identifier: GPL-2.0
#
# Makefile for HWMON Demo Driver
#

obj-m := hwmon_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod hwmon_demo.ko

unload:
	sudo rmmod hwmon_demo

test:
	@echo "=== HWMON Demo Test ==="
	@HWMON=$$(ls -d /sys/class/hwmon/hwmon* 2>/dev/null | while read h; do \
		if [ "$$(cat $$h/name 2>/dev/null)" = "demo_hwmon" ]; then \
			echo $$h; break; \
		fi; \
	done); \
	if [ -z "$$HWMON" ]; then \
		echo "Demo HWMON not found. Is the module loaded?"; \
		exit 1; \
	fi; \
	echo "Found: $$HWMON"; \
	echo ""; \
	echo "Temperature Sensors:"; \
	for i in 1 2; do \
		if [ -f "$$HWMON/temp$${i}_input" ]; then \
			label=$$(cat $$HWMON/temp$${i}_label 2>/dev/null || echo "temp$$i"); \
			val=$$(cat $$HWMON/temp$${i}_input); \
			echo "  $$label: $$((val/1000)).$$((val%1000/100))°C"; \
		fi; \
	done; \
	echo ""; \
	echo "Voltage Sensors:"; \
	for i in 0 1; do \
		if [ -f "$$HWMON/in$${i}_input" ]; then \
			label=$$(cat $$HWMON/in$${i}_label 2>/dev/null || echo "in$$i"); \
			val=$$(cat $$HWMON/in$${i}_input); \
			echo "  $$label: $$((val/1000)).$$((val%1000))V"; \
		fi; \
	done; \
	echo ""; \
	echo "Fan Sensors:"; \
	for i in 1; do \
		if [ -f "$$HWMON/fan$${i}_input" ]; then \
			label=$$(cat $$HWMON/fan$${i}_label 2>/dev/null || echo "fan$$i"); \
			val=$$(cat $$HWMON/fan$${i}_input); \
			echo "  $$label: $${val} RPM"; \
		fi; \
	done

.PHONY: all clean load unload test
