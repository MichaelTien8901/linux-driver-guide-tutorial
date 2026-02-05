# Device Tree Platform Driver Demo Makefile

obj-m := dt_platform_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install: modules
	sudo insmod dt_platform_demo.ko

uninstall:
	sudo rmmod dt_platform_demo

info:
	@echo "=== Module Info ==="
	@modinfo dt_platform_demo.ko 2>/dev/null || echo "Module not built"
	@echo ""
	@echo "=== Loaded Status ==="
	@lsmod | grep dt_platform || echo "Module not loaded"
	@echo ""
	@echo "=== Compatible Strings ==="
	@modinfo dt_platform_demo.ko 2>/dev/null | grep alias || echo "N/A"

# Note: This module requires a Device Tree entry to probe
# See example-device.dts for an example node
# On systems without DT, the module loads but no device is probed

help:
	@echo "Device Tree Platform Driver Demo - Build Targets"
	@echo ""
	@echo "  make          - Build the kernel module"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make install  - Load the module"
	@echo "  make uninstall - Unload the module"
	@echo "  make info     - Show module information"
	@echo ""
	@echo "Usage:"
	@echo "  1. Build: make"
	@echo "  2. Add device tree node (see example-device.dts)"
	@echo "  3. Load: sudo insmod dt_platform_demo.ko"
	@echo "  4. View: cat /proc/dt_platform_demo"
	@echo ""
	@echo "Note: Without a matching Device Tree node, the driver loads"
	@echo "      but probe() won't be called."

.PHONY: all modules clean install uninstall info help
