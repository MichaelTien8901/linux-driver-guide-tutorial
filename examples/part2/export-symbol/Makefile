# SPDX-License-Identifier: GPL-2.0
#
# Build two modules: mylib.ko (exports symbols) and myuser.ko (uses them)

obj-m := mylib.o myuser.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

# Helper targets for testing
load:
	sudo insmod mylib.ko
	sudo insmod myuser.ko

unload:
	sudo rmmod myuser
	sudo rmmod mylib

reload: unload load

.PHONY: all clean load unload reload
