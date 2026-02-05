# SPDX-License-Identifier: GPL-2.0
#
# IOCTL example device Makefile

obj-m := ioctl_device.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Kernel module
all: modules test_ioctl

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f test_ioctl

# User space test program
test_ioctl: test_ioctl.c ioctl_example.h
	$(CC) -o test_ioctl test_ioctl.c

# Helper targets
load:
	sudo insmod ioctl_device.ko
	@echo "Device created at /dev/ioctl_example"

unload:
	sudo rmmod ioctl_device

reload: unload load

test: test_ioctl
	@echo "Running IOCTL tests..."
	sudo ./test_ioctl

.PHONY: all modules clean load unload reload test
