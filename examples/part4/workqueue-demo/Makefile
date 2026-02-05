# SPDX-License-Identifier: GPL-2.0
#
# Work queue demonstration Makefile

obj-m := workqueue_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

load:
	sudo insmod workqueue_demo.ko
	@echo "Check /proc/workqueue_demo for stats"

unload:
	sudo rmmod workqueue_demo

reload: unload load

stats:
	@cat /proc/workqueue_demo

immediate:
	@echo "immediate" | sudo tee /proc/workqueue_demo > /dev/null
	@echo "Queued immediate work"

delayed:
	@echo "delayed" | sudo tee /proc/workqueue_demo > /dev/null
	@echo "Queued delayed work (2s)"

custom:
	@echo "custom" | sudo tee /proc/workqueue_demo > /dev/null
	@echo "Queued custom work"

start-periodic:
	@echo "start" | sudo tee /proc/workqueue_demo > /dev/null
	@echo "Started periodic work"

stop-periodic:
	@echo "stop" | sudo tee /proc/workqueue_demo > /dev/null
	@echo "Stopped periodic work"

test:
	@echo "Loading module..."
	sudo insmod workqueue_demo.ko
	@echo ""
	@echo "Initial stats:"
	@cat /proc/workqueue_demo
	@echo ""
	@echo "Queuing immediate work..."
	@echo "immediate" | sudo tee /proc/workqueue_demo > /dev/null
	@sleep 1
	@echo ""
	@echo "Queuing delayed work (2s)..."
	@echo "delayed" | sudo tee /proc/workqueue_demo > /dev/null
	@echo "Queuing custom work..."
	@echo "custom" | sudo tee /proc/workqueue_demo > /dev/null
	@echo "custom" | sudo tee /proc/workqueue_demo > /dev/null
	@echo ""
	@echo "Waiting 3s for delayed work..."
	@sleep 3
	@echo ""
	@echo "Stats after work:"
	@cat /proc/workqueue_demo
	@echo ""
	@echo "Starting periodic work..."
	@echo "start" | sudo tee /proc/workqueue_demo > /dev/null
	@sleep 3
	@echo ""
	@echo "Stats after 3s periodic:"
	@cat /proc/workqueue_demo
	@echo ""
	@echo "Stopping periodic..."
	@echo "stop" | sudo tee /proc/workqueue_demo > /dev/null
	@echo ""
	@echo "Unloading module..."
	sudo rmmod workqueue_demo
	@echo "Done! Check dmesg for details."

.PHONY: all clean load unload reload stats immediate delayed custom \
        start-periodic stop-periodic test
