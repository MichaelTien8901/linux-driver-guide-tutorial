# SPDX-License-Identifier: GPL-2.0
#
# Makefile for RTC Demo Driver
#

obj-m := rtc_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod rtc_demo.ko

unload:
	sudo rmmod rtc_demo

test:
	@echo "=== RTC Demo Test ==="
	@RTC=$$(ls -d /sys/class/rtc/rtc* 2>/dev/null | while read d; do \
		if [ "$$(cat $$d/name 2>/dev/null)" = "demo-rtc" ]; then \
			basename $$d; break; \
		fi; \
	done); \
	if [ -z "$$RTC" ]; then \
		echo "Demo RTC not found. Is the module loaded?"; \
		exit 1; \
	fi; \
	echo "Found: /dev/$$RTC"; \
	echo ""; \
	echo "Reading time:"; \
	sudo hwclock -r -f /dev/$$RTC; \
	echo ""; \
	echo "RTC sysfs attributes:"; \
	echo "  Name: $$(cat /sys/class/rtc/$$RTC/name)"; \
	echo "  Date: $$(cat /sys/class/rtc/$$RTC/date)"; \
	echo "  Time: $$(cat /sys/class/rtc/$$RTC/time)"; \
	echo ""; \
	echo "Setting alarm for 5 seconds from now:"; \
	ALARM_TIME=$$(date -d '+5 seconds' '+%Y-%m-%d %H:%M:%S'); \
	echo "  Alarm: $$ALARM_TIME"; \
	sudo sh -c "echo 0 > /sys/class/rtc/$$RTC/wakealarm"; \
	sudo sh -c "echo \"+5\" > /sys/class/rtc/$$RTC/wakealarm" 2>/dev/null || \
		echo "  (wakealarm write may require additional setup)"; \
	echo ""; \
	echo "Test complete!"

.PHONY: all clean load unload test
