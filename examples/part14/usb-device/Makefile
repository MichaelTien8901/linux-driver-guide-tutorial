# SPDX-License-Identifier: GPL-2.0
#
# Makefile for USB Demo Driver
#

obj-m := usb_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KDIR ?= $(KERNEL_DIR)
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod usb_demo.ko

unload:
	sudo rmmod usb_demo

info:
	@echo "=== USB Device Info ==="
	@echo "This driver matches VID:PID 1234:5678"
	@echo ""
	@echo "To test with a real device:"
	@echo "  1. Modify DEMO_VID and DEMO_PID in usb_demo.c"
	@echo "  2. Rebuild: make clean && make"
	@echo "  3. Load: sudo insmod usb_demo.ko"
	@echo "  4. Check: ls -la /dev/usb_demo"
	@echo ""
	@echo "List USB devices: lsusb"

.PHONY: all clean load unload info
