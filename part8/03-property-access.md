---
layout: default
title: "8.3 Property Access"
parent: "Part 8: Platform Bus and Device Tree"
nav_order: 3
---

# Property Access

This chapter covers the APIs for reading properties from Device Tree nodes in your driver code.

## Two API Families

Linux provides two ways to access device properties:

| API Family | Prefix | Scope |
|------------|--------|-------|
| Device Tree specific | `of_property_*` | DT only |
| Unified device property | `device_property_*` | DT, ACPI, platform data |

**Recommendation**: Use `device_property_*` for portability unless you need DT-specific features.

## Reading Integer Properties

### Single Values

```dts
device@10000000 {
    reg = <0x10000000 0x1000>;
    vendor,buffer-size = <4096>;
    vendor,timeout-ms = <100>;
};
```

```c
#include <linux/property.h>

static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    u32 buffer_size, timeout;
    int ret;

    /* Read required property */
    ret = device_property_read_u32(dev, "vendor,buffer-size", &buffer_size);
    if (ret) {
        dev_err(dev, "Missing buffer-size property\n");
        return ret;
    }

    /* Read optional property with default */
    ret = device_property_read_u32(dev, "vendor,timeout-ms", &timeout);
    if (ret)
        timeout = 1000;  /* Default 1 second */

    dev_info(dev, "Buffer: %u, Timeout: %u ms\n", buffer_size, timeout);
    return 0;
}
```

### Integer Arrays

```dts
device@10000000 {
    vendor,thresholds = <10 20 50 100>;
};
```

```c
static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    u32 thresholds[4];
    int ret, i;

    /* Read array */
    ret = device_property_read_u32_array(dev, "vendor,thresholds",
                                         thresholds, ARRAY_SIZE(thresholds));
    if (ret) {
        dev_err(dev, "Failed to read thresholds\n");
        return ret;
    }

    for (i = 0; i < ARRAY_SIZE(thresholds); i++)
        dev_info(dev, "Threshold[%d]: %u\n", i, thresholds[i]);

    return 0;
}
```

### Different Integer Sizes

```c
/* 8-bit */
u8 val8;
device_property_read_u8(dev, "prop", &val8);
device_property_read_u8_array(dev, "prop", arr, count);

/* 16-bit */
u16 val16;
device_property_read_u16(dev, "prop", &val16);
device_property_read_u16_array(dev, "prop", arr, count);

/* 32-bit */
u32 val32;
device_property_read_u32(dev, "prop", &val32);
device_property_read_u32_array(dev, "prop", arr, count);

/* 64-bit */
u64 val64;
device_property_read_u64(dev, "prop", &val64);
device_property_read_u64_array(dev, "prop", arr, count);
```

## Reading Strings

### Single String

```dts
device@10000000 {
    vendor,mode = "high-speed";
};
```

```c
static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    const char *mode;
    int ret;

    ret = device_property_read_string(dev, "vendor,mode", &mode);
    if (ret) {
        mode = "normal";  /* Default */
    }

    dev_info(dev, "Mode: %s\n", mode);
    return 0;
}
```

### String Arrays

```dts
device@10000000 {
    clock-names = "core", "bus", "phy";
};
```

```c
static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    const char *names[3];
    int ret, i;

    ret = device_property_read_string_array(dev, "clock-names",
                                            names, ARRAY_SIZE(names));
    if (ret < 0)
        return ret;

    for (i = 0; i < ret; i++)
        dev_info(dev, "Clock %d: %s\n", i, names[i]);

    return 0;
}
```

### Matching String in List

```c
/* Check if string matches one in array */
int idx = device_property_match_string(dev, "vendor,modes", "fast");
if (idx >= 0)
    dev_info(dev, "Found 'fast' at index %d\n", idx);
```

## Boolean Properties

```dts
device@10000000 {
    vendor,feature-enabled;  /* Present = true */
    /* vendor,other-feature not present = false */
};
```

```c
static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    bool feature_enabled;

    feature_enabled = device_property_read_bool(dev, "vendor,feature-enabled");
    /* Returns true if property exists, false otherwise */

    if (feature_enabled)
        dev_info(dev, "Feature is enabled\n");

    return 0;
}
```

## Checking Property Existence

```c
/* Check if property exists */
if (device_property_present(dev, "vendor,optional-prop")) {
    /* Property exists */
}

/* Count array elements */
int count = device_property_count_u32(dev, "vendor,values");
if (count > 0) {
    u32 *values = kmalloc_array(count, sizeof(u32), GFP_KERNEL);
    device_property_read_u32_array(dev, "vendor,values", values, count);
}
```

## Device Tree Specific API

When you need DT-specific functionality:

```c
#include <linux/of.h>

static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    u32 value;
    const char *str;

    /* Check if from Device Tree */
    if (!np) {
        dev_err(&pdev->dev, "No Device Tree node\n");
        return -ENODEV;
    }

    /* Read properties using of_* API */
    of_property_read_u32(np, "vendor,prop", &value);
    of_property_read_string(np, "vendor,name", &str);

    /* Check property existence */
    if (of_property_read_bool(np, "vendor,feature"))
        enable_feature();

    return 0;
}
```

### Reading Raw Data

```c
/* Get property as raw data */
const void *data;
int len;

data = of_get_property(np, "vendor,blob", &len);
if (data)
    process_blob(data, len);

/* For struct */
struct of_property *prop = of_find_property(np, "vendor,data", NULL);
```

## phandle References

### Reading phandle

```dts
device@10000000 {
    vendor,external = <&other_device>;
};

other_device: other@20000000 {
    compatible = "vendor,other";
};
```

```c
#include <linux/of.h>

static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct device_node *other_np;

    /* Get referenced node */
    other_np = of_parse_phandle(np, "vendor,external", 0);
    if (!other_np) {
        dev_err(&pdev->dev, "Missing external reference\n");
        return -ENODEV;
    }

    dev_info(&pdev->dev, "External device: %pOF\n", other_np);

    /* Important: Release reference when done */
    of_node_put(other_np);

    return 0;
}
```

### phandle with Arguments

```dts
device@10000000 {
    vendor,channel = <&mux 5>;  /* mux device, channel 5 */
};

mux: mux@30000000 {
    #vendor,mux-cells = <1>;
};
```

```c
struct of_phandle_args args;
int ret;

ret = of_parse_phandle_with_args(np, "vendor,channel",
                                  "#vendor,mux-cells", 0, &args);
if (ret == 0) {
    dev_info(dev, "Mux node: %pOF, channel: %d\n",
             args.np, args.args[0]);
    of_node_put(args.np);
}
```

## Child Nodes

```dts
device@10000000 {
    child@0 {
        reg = <0>;
        vendor,type = "input";
    };
    child@1 {
        reg = <1>;
        vendor,type = "output";
    };
};
```

```c
static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct device_node *child;
    int count = 0;

    /* Count children */
    count = of_get_child_count(np);
    dev_info(&pdev->dev, "Found %d children\n", count);

    /* Iterate over children */
    for_each_child_of_node(np, child) {
        u32 reg;
        const char *type;

        of_property_read_u32(child, "reg", &reg);
        of_property_read_string(child, "vendor,type", &type);

        dev_info(&pdev->dev, "Child %u: type=%s\n", reg, type);
    }

    /* Get specific child by name */
    child = of_get_child_by_name(np, "child@0");
    if (child) {
        /* Use child */
        of_node_put(child);
    }

    return 0;
}
```

## Error Handling Pattern

```c
static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    u32 required_val;
    u32 optional_val;
    const char *mode;
    int ret;

    /* Required property */
    ret = device_property_read_u32(dev, "vendor,required", &required_val);
    if (ret)
        return dev_err_probe(dev, ret, "Missing required property\n");

    /* Optional with default */
    if (device_property_read_u32(dev, "vendor,optional", &optional_val))
        optional_val = DEFAULT_VALUE;

    /* Optional string */
    ret = device_property_read_string(dev, "vendor,mode", &mode);
    if (ret)
        mode = "default";

    return 0;
}
```

## Summary

| Function | Purpose |
|----------|---------|
| `device_property_read_u32()` | Read single 32-bit value |
| `device_property_read_u32_array()` | Read array of 32-bit values |
| `device_property_read_string()` | Read string |
| `device_property_read_bool()` | Check boolean property |
| `device_property_present()` | Check if property exists |
| `device_property_count_*()` | Count array elements |

- Use `device_property_*` for portable code
- Use `of_*` when DT-specific features needed
- Always handle missing optional properties with defaults
- Release node references with `of_node_put()`

## Next

Learn about [GPIO references]({% link part8/04-gpio-references.md %}) in Device Tree.
