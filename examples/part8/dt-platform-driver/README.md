# Device Tree Platform Driver Demo

Demonstrates a platform driver that uses Device Tree for configuration, from Part 8.

## Features

- **Device Tree Matching**: Uses `of_device_id` table with compatible strings
- **Variant Data**: Different configurations based on compatible string
- **Property Reading**: Required, optional, and boolean properties
- **Resource Acquisition**: GPIOs, clocks, regulators from DT
- **Child Node Parsing**: Iterates child nodes for channel configuration

## Files

| File | Description |
|------|-------------|
| `dt_platform_demo.c` | Driver source code |
| `Makefile` | Build configuration |
| `example-device.dts` | Example Device Tree node |
| `README.md` | This file |

## Building

```bash
make
```

## Device Tree Node

The driver matches these compatible strings:
- `demo,dt-platform-v1` - Basic variant (4 channels, no DMA)
- `demo,dt-platform-v2` - Advanced variant (8 channels, with DMA)

### Required Properties

| Property | Type | Description |
|----------|------|-------------|
| `compatible` | string | Must match driver |
| `demo,buffer-size` | u32 | Buffer size in bytes |

### Optional Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `demo,timeout-ms` | u32 | 1000 | Timeout in milliseconds |
| `demo,mode` | string | "normal" | Operating mode |
| `demo,feature-enable` | bool | false | Enable advanced feature |
| `reset-gpios` | GPIO | - | Reset GPIO |
| `enable-gpios` | GPIO | - | Enable GPIO |
| `clocks` | phandle | - | Clock reference |
| `vdd-supply` | phandle | - | Power supply |

### Child Nodes (Channels)

```dts
channel@N {
    reg = <N>;              /* Required: channel number */
    label = "name";         /* Optional: human-readable name */
    demo,mode = "mode";     /* Optional: channel mode */
};
```

## Example Device Tree

```dts
demo_device: demo@10000000 {
    compatible = "demo,dt-platform-v2";
    demo,buffer-size = <4096>;
    demo,timeout-ms = <500>;
    demo,mode = "fast";
    demo,feature-enable;

    reset-gpios = <&gpio0 5 GPIO_ACTIVE_LOW>;

    channel@0 {
        reg = <0>;
        label = "temperature";
        demo,mode = "continuous";
    };

    channel@1 {
        reg = <1>;
        label = "pressure";
    };
};
```

## Testing

### On Systems with Device Tree

1. Add the device node to your board's DTS file
2. Rebuild and load the DTB
3. Load the module:

```bash
sudo insmod dt_platform_demo.ko
cat /proc/dt_platform_demo
```

### On Systems without Device Tree (x86)

The driver loads but won't probe any devices:

```bash
sudo insmod dt_platform_demo.ko
# No /proc/dt_platform_demo since no device was probed
dmesg | grep dt_platform
```

### Using Device Tree Overlay

On systems supporting overlays (Raspberry Pi, BeagleBone):

```bash
# Compile overlay
dtc -I dts -O dtb -@ -o demo_overlay.dtbo example-device.dts

# Load overlay (platform-specific)
# Raspberry Pi: add to /boot/config.txt
# BeagleBone: use /sys/kernel/config/device-tree/overlays/
```

## Code Walkthrough

### Matching

```c
static const struct of_device_id dt_demo_of_match[] = {
    { .compatible = "demo,dt-platform-v1", .data = &variant_v1 },
    { .compatible = "demo,dt-platform-v2", .data = &variant_v2 },
    { }
};
```

### Getting Variant Data

```c
variant = of_device_get_match_data(&pdev->dev);
```

### Reading Properties

```c
/* Required property */
ret = device_property_read_u32(dev, "demo,buffer-size", &val);
if (ret)
    return ret;  /* Error: property missing */

/* Optional with default */
if (device_property_read_u32(dev, "demo,timeout-ms", &val))
    val = 1000;  /* Default */

/* Boolean */
enabled = device_property_read_bool(dev, "demo,feature-enable");
```

### Getting Optional Resources

```c
/* Optional GPIO */
gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
if (IS_ERR(gpio))
    return PTR_ERR(gpio);
/* gpio is NULL if not present, valid descriptor if present */

/* Optional clock */
clk = devm_clk_get_optional(dev, "demo");

/* Optional regulator */
reg = devm_regulator_get_optional(dev, "vdd");
if (IS_ERR(reg)) {
    if (PTR_ERR(reg) == -ENODEV)
        reg = NULL;  /* Not present, OK */
    else
        return PTR_ERR(reg);  /* Other error */
}
```

### Parsing Child Nodes

```c
for_each_child_of_node(np, child) {
    of_property_read_u32(child, "reg", &reg);
    of_property_read_string(child, "label", &label);
    /* ... */
}
```

## Output Example

```
$ cat /proc/dt_platform_demo
Device Tree Platform Driver Demo
================================

Variant:         dt-demo-v2
Max channels:    8
Has DMA:         yes

Properties from Device Tree:
  buffer-size:   4096 bytes
  timeout-ms:    500 ms
  mode:          fast
  feature:       enabled

Resources:
  Reset GPIO:    present
  Enable GPIO:   not present
  Clock:         not present
  Regulator:     not present

Channels: 2
  Channel 0:
    reg:    0
    label:  temperature
    mode:   continuous
  Channel 1:
    reg:    1
    label:  pressure
    mode:   default
```

## References

- [Part 8.1: Device Tree Basics](../../../part8/01-devicetree-basics.md)
- [Part 8.2: Device Bindings](../../../part8/02-bindings.md)
- [Part 8.3: Property Access](../../../part8/03-property-access.md)
