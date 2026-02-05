# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Hello World kernel module
#
# Usage:
#   make                           # Build for host kernel
#   make KERNEL_DIR=/kernel/linux  # Build against specific kernel
#   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-  # Cross-compile
#   make clean                     # Remove build artifacts

# Module name (produces hello.ko)
obj-m := hello.o

# Kernel build directory
# Default to currently running kernel headers
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

# Current directory (where this Makefile lives)
PWD := $(shell pwd)

# Default target: build the module
all: modules

# Build kernel module
modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# Remove build artifacts
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.order *.symvers .*.cmd

# Show help
help:
	@echo "Hello World Kernel Module"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the module (default)"
	@echo "  clean   - Remove build artifacts"
	@echo "  help    - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  KERNEL_DIR     - Path to kernel build directory"
	@echo "  ARCH           - Target architecture (arm64, arm, x86)"
	@echo "  CROSS_COMPILE  - Cross-compiler prefix"
	@echo ""
	@echo "Examples:"
	@echo "  make"
	@echo "  make KERNEL_DIR=/kernel/linux"
	@echo "  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KERNEL_DIR=/kernel/linux"

.PHONY: all modules clean help
