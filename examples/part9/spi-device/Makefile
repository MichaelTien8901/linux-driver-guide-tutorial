# SPDX-License-Identifier: GPL-2.0
#
# Makefile for SPI Flash Demo Driver
#

obj-m := spi_flash_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
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
	sudo insmod spi_flash_demo.ko

# Unload module (requires root)
unload:
	sudo rmmod spi_flash_demo

# Show kernel log
log:
	sudo dmesg | tail -20

.PHONY: all clean install load unload log
