# SPDX-License-Identifier: GPL-2.0
#
# kmem_cache demonstration Makefile

obj-m := kmem_cache_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

load:
	sudo insmod kmem_cache_demo.ko
	@echo "Check /proc/kmem_cache_demo for status"

unload:
	sudo rmmod kmem_cache_demo

reload: unload load

stats:
	@cat /proc/kmem_cache_demo

alloc:
	@echo "alloc test_data" | sudo tee /proc/kmem_cache_demo > /dev/null

test:
	@echo "Loading module..."
	sudo insmod kmem_cache_demo.ko
	@echo ""
	@echo "Allocating objects..."
	@echo "alloc object_one" | sudo tee /proc/kmem_cache_demo > /dev/null
	@echo "alloc object_two" | sudo tee /proc/kmem_cache_demo > /dev/null
	@echo "alloc object_three" | sudo tee /proc/kmem_cache_demo > /dev/null
	@echo ""
	@echo "Stats:"
	@cat /proc/kmem_cache_demo
	@echo ""
	@echo "Accessing object 1..."
	@echo "access 1" | sudo tee /proc/kmem_cache_demo > /dev/null
	@echo "access 1" | sudo tee /proc/kmem_cache_demo > /dev/null
	@echo ""
	@echo "Freeing object 0..."
	@echo "free 0" | sudo tee /proc/kmem_cache_demo > /dev/null
	@echo ""
	@echo "Final stats:"
	@cat /proc/kmem_cache_demo
	@echo ""
	@echo "Unloading..."
	sudo rmmod kmem_cache_demo
	@echo "Done! Check dmesg for details."

.PHONY: all clean load unload reload stats alloc test
