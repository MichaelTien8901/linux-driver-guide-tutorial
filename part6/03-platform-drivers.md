---
layout: default
title: "6.3 Platform Drivers"
parent: "Part 6: Device Model and Driver Framework"
nav_order: 3
---

# Platform Drivers

Platform drivers handle devices that are not discoverable by hardware enumeration. These include SoC peripherals, memory-mapped devices, and devices described in Device Tree.

## Platform Device Structure

```c
#include <linux/platform_device.h>

struct platform_device {
    const char *name;           /* Device name (used for matching) */
    int id;                     /* Device instance ID (-1 for single) */
    struct device dev;          /* Embedded device structure */
    u32 num_resources;          /* Number of resources */
    struct resource *resource;  /* I/O memory, IRQs, etc. */
    const struct platform_device_id *id_entry;
    /* ... */
};
```

## Platform Driver Structure

```c
struct platform_driver {
    int (*probe)(struct platform_device *);    /* Device binding */
    int (*remove)(struct platform_device *);   /* Device unbinding */
    void (*shutdown)(struct platform_device *);
    int (*suspend)(struct platform_device *, pm_message_t state);
    int (*resume)(struct platform_device *);
    struct device_driver driver;               /* Embedded driver */
    const struct platform_device_id *id_table; /* ID matching */
};
```

## Basic Platform Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>

static int my_probe(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "Device probed\n");
    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "Device removed\n");
    return 0;
}

static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my_device",
        .owner = THIS_MODULE,
    },
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Basic platform driver");
```

## Matching Methods

### 1. Name Matching

```c
static struct platform_driver my_driver = {
    .driver = {
        .name = "my_device",  /* Matches platform_device.name */
    },
};

/* Platform device with matching name */
static struct platform_device my_pdev = {
    .name = "my_device",
    .id = -1,
};
```

### 2. ID Table Matching

```c
static const struct platform_device_id my_ids[] = {
    { "my_device_v1", (kernel_ulong_t)&v1_data },
    { "my_device_v2", (kernel_ulong_t)&v2_data },
    { }
};
MODULE_DEVICE_TABLE(platform, my_ids);

static struct platform_driver my_driver = {
    .id_table = my_ids,
    .driver = {
        .name = "my_device",
    },
};

/* Access matched data in probe */
static int my_probe(struct platform_device *pdev)
{
    const struct platform_device_id *id = platform_get_device_id(pdev);
    struct my_data *data = (struct my_data *)id->driver_data;
    /* ... */
}
```

### 3. Device Tree Matching (Recommended)

```c
static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,my-device-v1", .data = &v1_data },
    { .compatible = "vendor,my-device-v2", .data = &v2_data },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);

static struct platform_driver my_driver = {
    .driver = {
        .name = "my_device",
        .of_match_table = my_of_match,
    },
};

/* Access matched data in probe */
static int my_probe(struct platform_device *pdev)
{
    const struct of_device_id *match;

    match = of_match_device(my_of_match, &pdev->dev);
    if (match) {
        struct my_data *data = match->data;
        /* ... */
    }
    /* Or simpler: */
    struct my_data *data = of_device_get_match_data(&pdev->dev);
}
```

## Accessing Resources

### Memory Regions

```c
static int my_probe(struct platform_device *pdev)
{
    struct resource *res;
    void __iomem *regs;

    /* Method 1: Get resource by type and index */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
        return -ENODEV;

    regs = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(regs))
        return PTR_ERR(regs);

    /* Method 2: Get by name (from device tree) */
    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "regs");

    return 0;
}
```

### Interrupts

```c
static int my_probe(struct platform_device *pdev)
{
    int irq;

    /* Method 1: Get IRQ by index */
    irq = platform_get_irq(pdev, 0);
    if (irq < 0)
        return irq;

    /* Method 2: Get by name */
    irq = platform_get_irq_byname(pdev, "tx_irq");

    /* Request IRQ */
    ret = devm_request_irq(&pdev->dev, irq, my_handler,
                           0, "my_device", dev);
    if (ret)
        return ret;

    return 0;
}
```

### Clocks

```c
#include <linux/clk.h>

static int my_probe(struct platform_device *pdev)
{
    struct clk *clk;

    clk = devm_clk_get(&pdev->dev, "main_clk");
    if (IS_ERR(clk))
        return PTR_ERR(clk);

    ret = clk_prepare_enable(clk);
    if (ret)
        return ret;

    /* Store for later use and disable */
    mydev->clk = clk;

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    clk_disable_unprepare(mydev->clk);
    return 0;
}
```

## Complete Platform Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>

struct my_device {
    void __iomem *regs;
    int irq;
    struct device *dev;
};

static irqreturn_t my_irq_handler(int irq, void *data)
{
    struct my_device *mydev = data;
    dev_dbg(mydev->dev, "IRQ received\n");
    return IRQ_HANDLED;
}

static int my_probe(struct platform_device *pdev)
{
    struct my_device *mydev;
    struct resource *res;
    int ret;

    /* Allocate driver data */
    mydev = devm_kzalloc(&pdev->dev, sizeof(*mydev), GFP_KERNEL);
    if (!mydev)
        return -ENOMEM;

    mydev->dev = &pdev->dev;

    /* Map registers */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    mydev->regs = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(mydev->regs))
        return PTR_ERR(mydev->regs);

    /* Get IRQ */
    mydev->irq = platform_get_irq(pdev, 0);
    if (mydev->irq < 0)
        return mydev->irq;

    /* Request IRQ */
    ret = devm_request_irq(&pdev->dev, mydev->irq, my_irq_handler,
                           0, pdev->name, mydev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ\n");
        return ret;
    }

    /* Store driver data */
    platform_set_drvdata(pdev, mydev);

    dev_info(&pdev->dev, "Device probed: regs=%p irq=%d\n",
             mydev->regs, mydev->irq);

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    struct my_device *mydev = platform_get_drvdata(pdev);

    dev_info(&pdev->dev, "Device removed\n");
    /* devm resources are automatically freed */

    return 0;
}

static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,my-device" },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);

static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my_device",
        .of_match_table = my_of_match,
    },
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Complete platform driver");
```

## Manual Device Registration

For testing without device tree:

```c
/* Define resources */
static struct resource my_resources[] = {
    {
        .start = 0x10000000,
        .end = 0x10000FFF,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = 42,
        .end = 42,
        .flags = IORESOURCE_IRQ,
    },
};

/* Create and register device */
static struct platform_device *pdev;

static int __init my_init(void)
{
    pdev = platform_device_register_simple("my_device", -1,
                                           my_resources,
                                           ARRAY_SIZE(my_resources));
    if (IS_ERR(pdev))
        return PTR_ERR(pdev);

    return platform_driver_register(&my_driver);
}

static void __exit my_exit(void)
{
    platform_driver_unregister(&my_driver);
    platform_device_unregister(pdev);
}
```

## Summary

- Platform drivers handle non-discoverable devices
- Match by name, ID table, or Device Tree compatible
- Use `platform_get_resource()` and `platform_get_irq()` for resources
- Prefer `devm_*` functions for automatic cleanup
- Use `module_platform_driver()` macro for simple registration
- Store driver data with `platform_set_drvdata()`

## Next

Learn about [probe and remove]({% link part6/04-probe-remove.md %}) - the driver lifecycle callbacks.
