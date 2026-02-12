# SPDX-License-Identifier: GPL-2.0
#
# UIO Demo Makefile

obj-m := uio_demo.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: modules uio_userspace

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

uio_userspace: uio_userspace.c
	$(CC) -o uio_userspace uio_userspace.c

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f uio_userspace

load:
	sudo insmod uio_demo.ko

unload:
	sudo rmmod uio_demo

reload: unload load

test: load uio_userspace
	@echo "=== UIO Demo Test ==="
	@sudo ./uio_userspace
	@echo "=== Test Complete ==="

.PHONY: all modules clean load unload reload test
