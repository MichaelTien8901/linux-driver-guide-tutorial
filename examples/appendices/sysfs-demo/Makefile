# SPDX-License-Identifier: GPL-2.0
obj-m := sysfs_demo.o
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

test:
	@echo "=== sysfs Demo Test ==="
	@SYSFS=/sys/devices/platform/sysfs_demo; \
	if [ ! -d "$$SYSFS" ]; then echo "Load module first"; exit 1; fi; \
	echo "Read status:"; cat $$SYSFS/status; \
	echo "Read value:"; cat $$SYSFS/value; \
	echo "Write value:"; echo 100 | sudo tee $$SYSFS/value; \
	echo "Read again:"; cat $$SYSFS/status

.PHONY: all clean test
