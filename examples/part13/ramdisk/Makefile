# SPDX-License-Identifier: GPL-2.0
#
# Makefile for RAM Disk Demo
#

obj-m := ramdisk_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod ramdisk_demo.ko

unload:
	sudo rmmod ramdisk_demo

test:
	@echo "=== RAM Disk Demo Test ==="
	@if [ ! -e /dev/ramdemo0 ]; then \
		echo "Error: /dev/ramdemo0 not found. Is module loaded?"; \
		exit 1; \
	fi
	@echo "1. Device info:"
	@lsblk /dev/ramdemo0 2>/dev/null || echo "   (lsblk not available)"
	@echo ""
	@echo "2. Create filesystem:"
	sudo mkfs.ext4 -F /dev/ramdemo0
	@echo ""
	@echo "3. Mount and test:"
	sudo mkdir -p /mnt/ramdemo
	sudo mount /dev/ramdemo0 /mnt/ramdemo
	@echo "   Writing test file..."
	echo "Hello from RAM disk!" | sudo tee /mnt/ramdemo/test.txt
	@echo "   Reading test file..."
	cat /mnt/ramdemo/test.txt
	@echo ""
	@echo "4. Disk usage:"
	df -h /mnt/ramdemo
	@echo ""
	@echo "5. Cleanup:"
	sudo umount /mnt/ramdemo
	sudo rmdir /mnt/ramdemo
	@echo ""
	@echo "Test complete!"

.PHONY: all clean load unload test
