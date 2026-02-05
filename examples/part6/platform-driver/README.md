# Platform Driver Demo

A complete platform driver implementation demonstrating the Linux device model and driver framework concepts from Part 6.

## Features

- **Device Matching**: Supports both Device Tree (`compatible` string) and platform ID table matching
- **Managed Resources**: Uses `devm_*` functions for automatic cleanup
- **Device Attributes**: Exposes read-write and read-only sysfs attributes
- **Power Management**: Implements system sleep and runtime PM callbacks
- **Device Variants**: Shows how to handle multiple hardware variants with a single driver

## Files

| File | Description |
|------|-------------|
| `platform_driver_demo.c` | Main driver source code |
| `Makefile` | Build configuration |
| `test_platform_driver.sh` | Test and demonstration script |
| `README.md` | This file |

## Building

```bash
# Build the module
make

# View module information
make info
```

## Loading and Testing

```bash
# Load the module
sudo insmod platform_driver_demo.ko

# Verify driver is registered
ls /sys/bus/platform/drivers/platform_demo/

# Check kernel messages
dmesg | tail

# Unload module
sudo rmmod platform_driver_demo
```

## Device Tree Example

To use this driver with a real device, add a node to your Device Tree:

```dts
/* Basic variant */
my_device@10000000 {
    compatible = "demo,platform-basic";
    reg = <0x10000000 0x1000>;
    clocks = <&clk_main>;
    clock-names = "main";
};

/* Advanced variant with DMA */
my_device@20000000 {
    compatible = "demo,platform-advanced";
    reg = <0x20000000 0x1000>;
    clocks = <&clk_main>;
    clock-names = "main";
};
```

## Sysfs Attributes

When a device is bound, these attributes appear under `/sys/devices/platform/<device>/`:

| Attribute | Mode | Description |
|-----------|------|-------------|
| `value` | RW | Integer value (0-1000) |
| `mode` | RW | Operating mode: idle, active, sleep |
| `enabled` | RW | Boolean enable/disable |
| `variant` | RO | Device variant name |
| `max_channels` | RO | Maximum supported channels |
| `has_dma` | RO | Whether device has DMA |

### Usage

```bash
# Read attributes (assuming device at /sys/devices/platform/platform-demo-basic.0)
cat /sys/devices/platform/platform-demo-basic.0/value
cat /sys/devices/platform/platform-demo-basic.0/mode
cat /sys/devices/platform/platform-demo-basic.0/variant

# Write attributes
echo 500 > /sys/devices/platform/platform-demo-basic.0/value
echo active > /sys/devices/platform/platform-demo-basic.0/mode
echo 1 > /sys/devices/platform/platform-demo-basic.0/enabled
```

## Code Walkthrough

### Driver Registration

```c
static struct platform_driver platform_demo_driver = {
    .probe = platform_demo_probe,
    .remove = platform_demo_remove,
    .id_table = platform_demo_id_table,
    .driver = {
        .name = "platform_demo",
        .of_match_table = platform_demo_of_match,
        .pm = &platform_demo_pm_ops,
        .dev_groups = platform_demo_groups,
    },
};

module_platform_driver(platform_demo_driver);
```

### Device Matching

The driver supports two matching methods:

1. **Device Tree** (via `of_match_table`):
   ```c
   static const struct of_device_id platform_demo_of_match[] = {
       { .compatible = "demo,platform-basic", .data = &variant_basic },
       { .compatible = "demo,platform-advanced", .data = &variant_advanced },
       { }
   };
   ```

2. **Platform ID Table** (for non-DT systems):
   ```c
   static const struct platform_device_id platform_demo_id_table[] = {
       { "platform-demo-basic", (kernel_ulong_t)&variant_basic },
       { "platform-demo-advanced", (kernel_ulong_t)&variant_advanced },
       { }
   };
   ```

### Probe Function

The probe function demonstrates:
- Getting variant data from match
- Allocating device-private structure with `devm_kzalloc()`
- Optional resource handling (memory, clocks)
- Using `dev_err_probe()` for proper error handling
- Registering cleanup actions with `devm_add_action_or_reset()`

### Sysfs Attributes

Uses the modern attribute group approach with automatic creation:

```c
static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(variant);

static struct attribute *platform_demo_attrs[] = {
    &dev_attr_value.attr,
    &dev_attr_variant.attr,
    NULL,
};
ATTRIBUTE_GROUPS(platform_demo);
```

Attributes are automatically created on probe via `.dev_groups` in the driver structure.

## Testing Without Hardware

Since this is a virtual device, you can test the driver by:

1. **Using QEMU** with a custom device tree
2. **Creating a helper module** that registers a `platform_device`
3. **Using Device Tree overlays** (if supported)

Example helper to create device programmatically:

```c
static struct platform_device *test_device;

static int __init test_init(void)
{
    test_device = platform_device_register_simple("platform-demo-basic", 0, NULL, 0);
    return PTR_ERR_OR_ZERO(test_device);
}

static void __exit test_exit(void)
{
    platform_device_unregister(test_device);
}
```

## Key Concepts Demonstrated

1. **Platform Driver Structure**: Complete `platform_driver` with all callbacks
2. **Device Matching**: Both DT and ID table methods
3. **Managed Resources**: `devm_*` functions for automatic cleanup
4. **Device Attributes**: sysfs interface with attribute groups
5. **Power Management**: System sleep and runtime PM
6. **Device Variants**: Single driver supporting multiple hardware variants
7. **Error Handling**: Using `dev_err_probe()` for EPROBE_DEFER

## References

- [Part 6.3: Platform Drivers](../../../part6/03-platform-drivers.md)
- [Part 6.4: Probe and Remove](../../../part6/04-probe-remove.md)
- [Part 6.5: Managed Resources](../../../part6/05-devres.md)
- [Part 6.6: Device Attributes](../../../part6/06-device-attributes.md)
- [Part 6.7: Deferred Probe](../../../part6/07-deferred-probe.md)
