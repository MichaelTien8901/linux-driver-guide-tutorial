# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Virtual Network Device Demo
#

obj-m := vnet_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod vnet_demo.ko

unload:
	sudo rmmod vnet_demo

test:
	@echo "=== Virtual Network Device Test ==="
	@IF=$$(ip -o link show | grep vnet | awk -F': ' '{print $$2}' | head -1); \
	if [ -z "$$IF" ]; then \
		echo "Error: vnet interface not found. Is module loaded?"; \
		exit 1; \
	fi; \
	echo "Found interface: $$IF"; \
	echo ""; \
	echo "1. Configure interface:"; \
	sudo ip addr add 10.0.99.1/24 dev $$IF 2>/dev/null || true; \
	sudo ip link set $$IF up; \
	ip addr show $$IF; \
	echo ""; \
	echo "2. Send test traffic (loopback):"; \
	ping -c 3 -I $$IF 10.0.99.1 2>/dev/null || echo "(ping may fail - that's OK for this demo)"; \
	echo ""; \
	echo "3. Check statistics:"; \
	ip -s link show $$IF; \
	echo ""; \
	echo "4. Send UDP packet:"; \
	echo "Hello from vnet!" | nc -u -w1 10.0.99.1 12345 2>/dev/null &; \
	sleep 1; \
	echo ""; \
	echo "5. Final statistics:"; \
	ip -s link show $$IF

.PHONY: all clean load unload test
