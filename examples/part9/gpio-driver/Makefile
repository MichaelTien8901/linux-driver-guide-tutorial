# SPDX-License-Identifier: GPL-2.0
#
# Makefile for GPIO Chip Demo Driver
#

obj-m := gpio_chip_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KDIR ?= $(KERNEL_DIR)
PWD := $(shell pwd)

# Default target
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Install module
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

# Load module (requires root)
load:
	sudo insmod gpio_chip_demo.ko

# Unload module (requires root)
unload:
	sudo rmmod gpio_chip_demo

# Show kernel log
log:
	sudo dmesg | tail -20

.PHONY: all clean install load unload log
