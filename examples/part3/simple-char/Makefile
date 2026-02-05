# SPDX-License-Identifier: GPL-2.0
#
# Simple character device driver Makefile

obj-m := simple_char.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

# Helper targets
load:
	sudo insmod simple_char.ko
	@echo "Device created at /dev/simple_char"

unload:
	sudo rmmod simple_char

reload: unload load

test:
	@echo "Testing simple_char device..."
	@echo "Writing 'Hello, driver!' to device..."
	@echo "Hello, driver!" | sudo tee /dev/simple_char > /dev/null
	@echo "Reading from device..."
	@sudo cat /dev/simple_char
	@echo ""
	@echo "Test complete!"

.PHONY: all clean load unload reload test
