# Platform Driver Demo Makefile

# Module name
obj-m := platform_driver_demo.o

# Kernel build directory
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KDIR ?= $(KERNEL_DIR)

# Current directory
PWD := $(shell pwd)

# Default target
all: modules

# Build kernel module
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Install module
install: modules
	sudo insmod platform_driver_demo.ko

# Remove module
uninstall:
	sudo rmmod platform_driver_demo

# Create test device (for testing without Device Tree)
create-device:
	@echo "Creating test platform device..."
	@echo 'platform-demo-basic.0' | sudo tee /sys/bus/platform/drivers/platform_demo/unbind 2>/dev/null || true
	@sudo sh -c 'echo "platform-demo-basic 0" > /sys/bus/platform/devices_probe' 2>/dev/null || \
		echo "Note: Device creation via devices_probe requires CONFIG_DEBUG_DRIVER"
	@echo "Alternative: Use the test script to create device via module parameter or sysfs"

# Show device information
info:
	@echo "=== Module Info ==="
	@modinfo platform_driver_demo.ko 2>/dev/null || echo "Module not built"
	@echo ""
	@echo "=== Loaded Status ==="
	@lsmod | grep platform_driver_demo || echo "Module not loaded"
	@echo ""
	@echo "=== Sysfs Entries ==="
	@ls -la /sys/bus/platform/drivers/platform_demo/ 2>/dev/null || echo "Driver not registered"

# Help target
help:
	@echo "Platform Driver Demo - Build Targets"
	@echo ""
	@echo "  make          - Build the kernel module"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make install  - Load the module"
	@echo "  make uninstall - Unload the module"
	@echo "  make info     - Show module and driver information"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Environment Variables:"
	@echo "  KDIR          - Kernel build directory (default: /lib/modules/\$$(uname -r)/build)"
	@echo ""
	@echo "Usage:"
	@echo "  1. Build:    make"
	@echo "  2. Load:     sudo insmod platform_driver_demo.ko"
	@echo "  3. Test:     See test_platform_driver.sh script"
	@echo "  4. Unload:   sudo rmmod platform_driver_demo"

.PHONY: all modules clean install uninstall create-device info help
