# Threaded IRQ Demo Makefile

obj-m := threaded_irq_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install: modules
	sudo insmod threaded_irq_demo.ko

uninstall:
	sudo rmmod threaded_irq_demo

test: install
	@echo "=== Starting threaded IRQ demo ==="
	@sleep 1
	@echo "Starting interrupt generation..."
	@echo "start" | sudo tee /proc/threaded_irq_demo/control
	@sleep 2
	@echo ""
	@echo "=== Statistics ==="
	@cat /proc/threaded_irq_demo/stats
	@echo ""
	@echo "=== Data Buffer ==="
	@cat /proc/threaded_irq_demo/data
	@echo ""
	@echo "Stopping..."
	@echo "stop" | sudo tee /proc/threaded_irq_demo/control

info:
	@echo "=== Module Info ==="
	@modinfo threaded_irq_demo.ko 2>/dev/null || echo "Module not built"
	@echo ""
	@echo "=== Loaded Status ==="
	@lsmod | grep threaded_irq || echo "Module not loaded"

help:
	@echo "Threaded IRQ Demo - Build Targets"
	@echo ""
	@echo "  make          - Build the kernel module"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make install  - Load the module"
	@echo "  make uninstall - Unload the module"
	@echo "  make test     - Run a quick demo"
	@echo "  make info     - Show module information"
	@echo ""
	@echo "Proc Interface:"
	@echo "  /proc/threaded_irq_demo/control - Control commands"
	@echo "  /proc/threaded_irq_demo/stats   - View statistics"
	@echo "  /proc/threaded_irq_demo/data    - View data buffer"

.PHONY: all modules clean install uninstall test info help
