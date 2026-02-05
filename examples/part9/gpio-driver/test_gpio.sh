#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Test script for GPIO Chip Demo Driver
#

set -e

DRIVER_NAME="gpio_chip_demo"
GPIO_CHIP_LABEL="demo-gpio"

echo "=== GPIO Chip Demo Driver Test ==="
echo

# Check if module is loaded
if ! lsmod | grep -q "$DRIVER_NAME"; then
    echo "Loading module..."
    sudo insmod ${DRIVER_NAME}.ko
    sleep 1
fi

# Find the GPIO chip
find_gpio_chip() {
    for chip in /sys/class/gpio/gpiochip*; do
        if [ -f "$chip/label" ]; then
            if grep -q "$GPIO_CHIP_LABEL" "$chip/label" 2>/dev/null; then
                echo "$chip"
                return 0
            fi
        fi
    done
    return 1
}

GPIO_CHIP=$(find_gpio_chip)
if [ -z "$GPIO_CHIP" ]; then
    echo "Error: Could not find demo GPIO chip"
    echo "Check: cat /sys/kernel/debug/gpio"
    exit 1
fi

# Get base GPIO number
GPIO_BASE=$(cat "$GPIO_CHIP/base")
GPIO_NGPIO=$(cat "$GPIO_CHIP/ngpio")

echo "Found GPIO chip: $GPIO_CHIP"
echo "  Base: $GPIO_BASE"
echo "  Number of GPIOs: $GPIO_NGPIO"
echo

# Function to export a GPIO
export_gpio() {
    local gpio=$1
    if [ ! -d "/sys/class/gpio/gpio$gpio" ]; then
        echo "$gpio" | sudo tee /sys/class/gpio/export > /dev/null 2>&1 || true
    fi
}

# Function to unexport a GPIO
unexport_gpio() {
    local gpio=$1
    if [ -d "/sys/class/gpio/gpio$gpio" ]; then
        echo "$gpio" | sudo tee /sys/class/gpio/unexport > /dev/null 2>&1 || true
    fi
}

echo "=== Test 1: GPIO Export/Unexport ==="
GPIO0=$((GPIO_BASE + 0))
export_gpio $GPIO0
if [ -d "/sys/class/gpio/gpio$GPIO0" ]; then
    echo "  PASS: GPIO $GPIO0 exported"
else
    echo "  FAIL: GPIO $GPIO0 not exported"
fi
unexport_gpio $GPIO0
echo

echo "=== Test 2: Direction Control ==="
GPIO1=$((GPIO_BASE + 1))
export_gpio $GPIO1

# Set as output
echo "out" | sudo tee /sys/class/gpio/gpio$GPIO1/direction > /dev/null
DIR=$(cat /sys/class/gpio/gpio$GPIO1/direction)
if [ "$DIR" = "out" ]; then
    echo "  PASS: GPIO $GPIO1 set to output"
else
    echo "  FAIL: Expected 'out', got '$DIR'"
fi

# Set as input
echo "in" | sudo tee /sys/class/gpio/gpio$GPIO1/direction > /dev/null
DIR=$(cat /sys/class/gpio/gpio$GPIO1/direction)
if [ "$DIR" = "in" ]; then
    echo "  PASS: GPIO $GPIO1 set to input"
else
    echo "  FAIL: Expected 'in', got '$DIR'"
fi

unexport_gpio $GPIO1
echo

echo "=== Test 3: Output Value ==="
GPIO2=$((GPIO_BASE + 2))
export_gpio $GPIO2
echo "out" | sudo tee /sys/class/gpio/gpio$GPIO2/direction > /dev/null

# Set high
echo "1" | sudo tee /sys/class/gpio/gpio$GPIO2/value > /dev/null
VAL=$(cat /sys/class/gpio/gpio$GPIO2/value)
if [ "$VAL" = "1" ]; then
    echo "  PASS: GPIO $GPIO2 set high"
else
    echo "  FAIL: Expected '1', got '$VAL'"
fi

# Set low
echo "0" | sudo tee /sys/class/gpio/gpio$GPIO2/value > /dev/null
VAL=$(cat /sys/class/gpio/gpio$GPIO2/value)
if [ "$VAL" = "0" ]; then
    echo "  PASS: GPIO $GPIO2 set low"
else
    echo "  FAIL: Expected '0', got '$VAL'"
fi

unexport_gpio $GPIO2
echo

echo "=== Test 4: Debugfs Interface ==="
if [ -f /sys/kernel/debug/demo_gpio ]; then
    echo "  Debugfs content:"
    sudo cat /sys/kernel/debug/demo_gpio | sed 's/^/    /'
else
    echo "  Note: Debugfs not available or not mounted"
fi
echo

echo "=== Test 5: GPIO Debug Info ==="
if [ -f /sys/kernel/debug/gpio ]; then
    echo "  System GPIO debug info (filtered):"
    sudo cat /sys/kernel/debug/gpio | grep -A 10 "demo-gpio" | sed 's/^/    /'
else
    echo "  Note: GPIO debug info not available"
fi
echo

echo "=== Test 6: libgpiod (if available) ==="
if command -v gpioinfo &> /dev/null; then
    echo "  GPIO chip info:"
    gpioinfo | grep -A 10 "demo-gpio" | head -15 | sed 's/^/    /'
else
    echo "  Note: libgpiod tools not installed"
    echo "  Install with: sudo apt-get install gpiod"
fi
echo

echo "=== Kernel Log ==="
sudo dmesg | grep -i "demo.gpio" | tail -10 | sed 's/^/  /'
echo

echo "=== Test Complete ==="
echo "To unload: sudo rmmod $DRIVER_NAME"
