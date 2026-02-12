#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Test script for Input Device Demo

set -e

DRIVER_NAME="input_demo"
echo "=== Input Device Demo Test ==="
echo

# Check/load module
if ! lsmod | grep -q "$DRIVER_NAME"; then
    echo "Loading module..."
    sudo insmod ${DRIVER_NAME}.ko
    sleep 1
fi

# Verify registration
echo "=== Test 1: Device Registration ==="
if cat /proc/bus/input/devices | grep -q "Virtual Button"; then
    echo "PASS: Input device registered"
else
    echo "FAIL: Input device not found"
    exit 1
fi
echo

# Trigger events
echo "=== Test 2: Key Events ==="
echo "click" | sudo tee /proc/input_demo > /dev/null
echo "click" | sudo tee /proc/input_demo > /dev/null
echo "power" | sudo tee /proc/input_demo > /dev/null
echo "PASS: Events triggered"
echo

# Check stats
echo "=== Test 3: Statistics ==="
cat /proc/input_demo
echo

# Show device info
echo "=== Device Info ==="
cat /proc/bus/input/devices | grep -A 6 "Virtual Button"
echo

# Show kernel log
echo "=== Kernel Log ==="
dmesg | grep input_demo | tail -10
echo

echo "=== Test Complete ==="
echo "To unload: sudo rmmod $DRIVER_NAME"
echo "Tip: use 'evtest' to monitor events in real time"
