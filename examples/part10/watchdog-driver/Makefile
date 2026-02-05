# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Watchdog Demo Driver
#

obj-m := watchdog_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KDIR ?= $(KERNEL_DIR)
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod watchdog_demo.ko

unload:
	sudo rmmod watchdog_demo

# Load with nowayout enabled
load-nowayout:
	sudo insmod watchdog_demo.ko nowayout=1

.PHONY: all clean load unload load-nowayout
