# SPDX-License-Identifier: GPL-2.0
#
# DMA demonstration Makefile

obj-m := dma_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

load:
	sudo insmod dma_demo.ko
	@echo "Check /proc/dma_demo for status"

unload:
	sudo rmmod dma_demo

reload: unload load

stats:
	@cat /proc/dma_demo

test:
	@echo "Loading module..."
	sudo insmod dma_demo.ko
	@echo ""
	@echo "Initial stats:"
	@cat /proc/dma_demo
	@echo ""
	@echo "Writing to coherent buffer..."
	@echo "cwrite hello_dma" | sudo tee /proc/dma_demo > /dev/null
	@echo ""
	@echo "Reading from coherent buffer..."
	@echo "cread" | sudo tee /proc/dma_demo > /dev/null
	@echo ""
	@echo "Mapping streaming buffer (TO_DEVICE)..."
	@echo "smap_to streaming_data" | sudo tee /proc/dma_demo > /dev/null
	@echo ""
	@echo "Stats after mapping:"
	@cat /proc/dma_demo
	@echo ""
	@echo "Unmapping streaming buffer..."
	@echo "sunmap_to" | sudo tee /proc/dma_demo > /dev/null
	@echo ""
	@echo "Final stats:"
	@cat /proc/dma_demo
	@echo ""
	@echo "Unloading module..."
	sudo rmmod dma_demo
	@echo "Done! Check dmesg for details."

.PHONY: all clean load unload reload stats test
