# SPDX-License-Identifier: GPL-2.0
#
# Spinlock demonstration Makefile

obj-m := spinlock_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

load:
	sudo insmod spinlock_demo.ko
	@echo "Check /proc/spinlock_demo for stats"

unload:
	sudo rmmod spinlock_demo

reload: unload load

stats:
	@cat /proc/spinlock_demo

test:
	@echo "Loading module..."
	sudo insmod spinlock_demo.ko
	@echo "Waiting 2 seconds for threads to run..."
	@sleep 2
	@echo "Stats:"
	@cat /proc/spinlock_demo
	@echo ""
	@echo "Waiting another 2 seconds..."
	@sleep 2
	@echo "Stats:"
	@cat /proc/spinlock_demo
	@echo ""
	@echo "Unloading module..."
	sudo rmmod spinlock_demo
	@echo "Check dmesg for thread messages"

.PHONY: all clean load unload reload stats test
