# SPDX-License-Identifier: GPL-2.0
#
# Makefile for PCI Skeleton Driver
#

obj-m := pci_skeleton.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

info:
	@echo "=== PCI Skeleton Driver ==="
	@echo ""
	@echo "This driver matches VID:PID 1234:5678"
	@echo ""
	@echo "To use with a real device:"
	@echo "  1. Find your device: lspci -nn"
	@echo "  2. Edit SKELETON_VID/PID in pci_skeleton.c"
	@echo "  3. Rebuild: make clean && make"
	@echo "  4. Load: sudo insmod pci_skeleton.ko"
	@echo "  5. Check: dmesg | tail"
	@echo ""
	@echo "List PCI devices:"
	@lspci -nn 2>/dev/null | head -10 || echo "  (lspci not available)"

.PHONY: all clean info
