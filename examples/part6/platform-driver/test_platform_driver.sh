#!/bin/bash
# Platform Driver Demo Test Script
#
# This script demonstrates creating a platform device and interacting
# with its sysfs attributes.

set -e

MODULE_NAME="platform_driver_demo"
DRIVER_NAME="platform_demo"
DEVICE_NAME="platform-demo-basic"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_header() {
    echo ""
    echo -e "${GREEN}=== $1 ===${NC}"
}

print_warning() {
    echo -e "${YELLOW}Warning: $1${NC}"
}

print_error() {
    echo -e "${RED}Error: $1${NC}"
}

print_success() {
    echo -e "${GREEN}$1${NC}"
}

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root"
        exit 1
    fi
}

# Load the module
load_module() {
    print_header "Loading Module"

    if lsmod | grep -q "$MODULE_NAME"; then
        echo "Module already loaded"
    else
        if [[ -f "${MODULE_NAME}.ko" ]]; then
            insmod "${MODULE_NAME}.ko"
            echo "Module loaded successfully"
        else
            print_error "Module file not found. Run 'make' first."
            exit 1
        fi
    fi

    # Verify driver is registered
    if [[ -d "/sys/bus/platform/drivers/${DRIVER_NAME}" ]]; then
        print_success "Driver registered at /sys/bus/platform/drivers/${DRIVER_NAME}"
    else
        print_error "Driver not found in sysfs"
        exit 1
    fi
}

# Create a platform device dynamically
# Note: This creates a simple device for testing purposes
create_device() {
    print_header "Creating Platform Device"

    local DEVICE_ID="${DEVICE_NAME}.0"

    # Check if device already exists
    if [[ -d "/sys/bus/platform/devices/${DEVICE_ID}" ]]; then
        echo "Device ${DEVICE_ID} already exists"
        return 0
    fi

    # For testing, we can create a device using a helper module
    # In production, devices come from Device Tree or ACPI
    print_warning "Creating device programmatically requires additional kernel support"
    echo "In real systems, platform devices are created from:"
    echo "  - Device Tree (embedded systems)"
    echo "  - ACPI tables (x86 systems)"
    echo "  - Board files (legacy)"
    echo ""
    echo "For testing, you can:"
    echo "  1. Use QEMU with a custom device tree"
    echo "  2. Create a helper module that registers a platform_device"
    echo "  3. Use the device tree overlay mechanism"

    # Try using configfs if available (requires CONFIG_OF_OVERLAY)
    if [[ -d "/sys/kernel/config/device-tree/overlays" ]]; then
        echo ""
        echo "Device Tree overlay support detected!"
        echo "You could create an overlay to instantiate the device."
    fi
}

# Test sysfs attributes (when device exists)
test_sysfs() {
    local DEVICE_PATH="$1"

    print_header "Testing Sysfs Attributes"

    if [[ ! -d "$DEVICE_PATH" ]]; then
        print_warning "Device path not found: $DEVICE_PATH"
        echo "Skipping sysfs tests"
        return 1
    fi

    echo "Device path: $DEVICE_PATH"
    echo ""

    # List available attributes
    echo "Available attributes:"
    ls -la "$DEVICE_PATH/" | grep -v "^d" | tail -n +2
    echo ""

    # Read attributes
    for attr in value mode enabled variant max_channels has_dma; do
        if [[ -f "${DEVICE_PATH}/${attr}" ]]; then
            local val=$(cat "${DEVICE_PATH}/${attr}")
            echo "  $attr = $val"
        fi
    done

    # Test writing attributes
    echo ""
    echo "Testing write operations:"

    if [[ -w "${DEVICE_PATH}/value" ]]; then
        echo "  Writing value=500..."
        echo 500 > "${DEVICE_PATH}/value"
        echo "  Read back: $(cat ${DEVICE_PATH}/value)"
    fi

    if [[ -w "${DEVICE_PATH}/mode" ]]; then
        echo "  Writing mode=active..."
        echo active > "${DEVICE_PATH}/mode"
        echo "  Read back: $(cat ${DEVICE_PATH}/mode)"
    fi

    if [[ -w "${DEVICE_PATH}/enabled" ]]; then
        echo "  Writing enabled=0..."
        echo 0 > "${DEVICE_PATH}/enabled"
        echo "  Read back: $(cat ${DEVICE_PATH}/enabled)"

        echo "  Writing enabled=1..."
        echo 1 > "${DEVICE_PATH}/enabled"
        echo "  Read back: $(cat ${DEVICE_PATH}/enabled)"
    fi

    print_success "Sysfs tests completed"
}

# Show driver information
show_info() {
    print_header "Driver Information"

    echo "Driver sysfs path: /sys/bus/platform/drivers/${DRIVER_NAME}"
    echo ""

    if [[ -d "/sys/bus/platform/drivers/${DRIVER_NAME}" ]]; then
        echo "Contents:"
        ls -la "/sys/bus/platform/drivers/${DRIVER_NAME}/"
        echo ""

        # Show bound devices
        echo "Bound devices:"
        for dev in /sys/bus/platform/drivers/${DRIVER_NAME}/*/; do
            if [[ -d "$dev" && ! "$dev" =~ "module" ]]; then
                basename "$dev"
            fi
        done 2>/dev/null || echo "  (none)"
    fi

    # Show module info
    echo ""
    echo "Module information:"
    if [[ -f "${MODULE_NAME}.ko" ]]; then
        modinfo "${MODULE_NAME}.ko" | head -15
    fi

    # Show dmesg output
    echo ""
    echo "Recent kernel messages:"
    dmesg | grep -i "platform_demo\|platform-demo" | tail -10 || echo "  (no messages)"
}

# Unload the module
unload_module() {
    print_header "Unloading Module"

    if lsmod | grep -q "$MODULE_NAME"; then
        rmmod "$MODULE_NAME"
        echo "Module unloaded successfully"
    else
        echo "Module not loaded"
    fi
}

# Main menu
usage() {
    echo "Platform Driver Demo Test Script"
    echo ""
    echo "Usage: $0 <command>"
    echo ""
    echo "Commands:"
    echo "  load      - Load the kernel module"
    echo "  unload    - Unload the kernel module"
    echo "  create    - Attempt to create a test device"
    echo "  test      - Test sysfs attributes (if device exists)"
    echo "  info      - Show driver and device information"
    echo "  all       - Run all tests (load, create, test, info)"
    echo "  clean     - Unload module and cleanup"
    echo ""
    echo "Example:"
    echo "  sudo $0 load"
    echo "  sudo $0 info"
    echo "  sudo $0 unload"
}

# Main
case "${1:-}" in
    load)
        check_root
        load_module
        ;;
    unload)
        check_root
        unload_module
        ;;
    create)
        check_root
        create_device
        ;;
    test)
        check_root
        # Try to find a bound device
        DEVICE_PATH=$(find /sys/bus/platform/devices -name "${DEVICE_NAME}*" -type l 2>/dev/null | head -1)
        if [[ -n "$DEVICE_PATH" ]]; then
            test_sysfs "$DEVICE_PATH"
        else
            print_warning "No device found. Device must be created via Device Tree or board code."
        fi
        ;;
    info)
        show_info
        ;;
    all)
        check_root
        load_module
        create_device
        show_info
        ;;
    clean)
        check_root
        unload_module
        ;;
    *)
        usage
        exit 1
        ;;
esac
