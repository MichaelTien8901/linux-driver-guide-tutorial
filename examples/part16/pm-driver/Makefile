# SPDX-License-Identifier: GPL-2.0
#
# Makefile for PM Demo Driver
#

obj-m := pm_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod pm_demo.ko

unload:
	sudo rmmod pm_demo

test:
	@echo "=== PM Demo Test ==="
	@SYSFS=/sys/devices/platform/pm_demo.0; \
	if [ ! -d "$$SYSFS" ]; then \
		echo "Error: Device not found. Is module loaded?"; \
		exit 1; \
	fi; \
	echo "1. Initial status:"; \
	cat $$SYSFS/status; \
	echo ""; \
	echo "2. Read data (triggers runtime resume):"; \
	cat $$SYSFS/data; \
	echo ""; \
	echo "3. Runtime status (should be active):"; \
	cat $$SYSFS/power/runtime_status; \
	echo ""; \
	echo "4. Waiting for autosuspend (3 seconds)..."; \
	sleep 3; \
	echo ""; \
	echo "5. Runtime status (should be suspended):"; \
	cat $$SYSFS/power/runtime_status; \
	echo ""; \
	echo "6. Write new data (triggers runtime resume):"; \
	echo 123 | sudo tee $$SYSFS/data; \
	echo ""; \
	echo "7. Final status:"; \
	cat $$SYSFS/status; \
	echo ""; \
	echo "8. Check kernel log:"; \
	dmesg | grep pm_demo | tail -10

.PHONY: all clean load unload test
