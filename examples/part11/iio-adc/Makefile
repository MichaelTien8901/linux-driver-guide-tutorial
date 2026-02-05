# SPDX-License-Identifier: GPL-2.0
#
# Makefile for IIO ADC Demo Driver
#

obj-m := iio_adc_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod iio_adc_demo.ko

unload:
	sudo rmmod iio_adc_demo

test:
	@echo "=== IIO ADC Demo Test ==="
	@IIO=$$(ls -d /sys/bus/iio/devices/iio:device* 2>/dev/null | while read d; do \
		if [ "$$(cat $$d/name 2>/dev/null)" = "demo-iio-adc" ]; then \
			echo $$d; break; \
		fi; \
	done); \
	if [ -z "$$IIO" ]; then \
		echo "Demo IIO ADC not found. Is the module loaded?"; \
		exit 1; \
	fi; \
	echo "Found: $$IIO"; \
	echo ""; \
	echo "ADC Channels:"; \
	for i in 0 1 2 3; do \
		raw=$$(cat $$IIO/in_voltage$${i}_raw 2>/dev/null); \
		scale=$$(cat $$IIO/in_voltage_scale 2>/dev/null); \
		if [ -n "$$raw" ] && [ -n "$$scale" ]; then \
			mv=$$(echo "$$raw * $$scale" | bc 2>/dev/null || echo "N/A"); \
			echo "  Channel $$i: raw=$$raw scale=$$scale mV=$$mv"; \
		fi; \
	done

.PHONY: all clean load unload test
