# SPDX-License-Identifier: GPL-2.0
#
# Data Structures Demo Makefile

obj-m := data_structures_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: modules

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

load:
	sudo insmod data_structures_demo.ko

unload:
	sudo rmmod data_structures_demo

reload: unload load

test: load
	@echo "=== Data Structures Demo Test ==="
	@echo "populate" | sudo tee /proc/data_structures_demo
	@cat /proc/data_structures_demo
	@echo "find 200" | sudo tee /proc/data_structures_demo
	@echo "clear" | sudo tee /proc/data_structures_demo
	@cat /proc/data_structures_demo
	@echo "=== Test Complete ==="

.PHONY: all modules clean load unload reload test
