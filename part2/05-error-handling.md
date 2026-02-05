---
layout: default
title: "2.5 Error Handling"
parent: "Part 2: Kernel Fundamentals"
nav_order: 5
---

# Error Handling Patterns

Proper error handling is critical in kernel code. A bug in a driver can crash the entire system.

## Return Value Conventions

### Function Return Values

Kernel functions return errors as **negative errno values**:

```c
#include <linux/errno.h>

int my_function(void)
{
        if (bad_input)
                return -EINVAL;   /* Invalid argument */

        if (no_memory)
                return -ENOMEM;   /* Out of memory */

        if (device_busy)
                return -EBUSY;    /* Device busy */

        return 0;  /* Success */
}
```

### Common Error Codes

| Code | Meaning | Common Use |
|------|---------|------------|
| `-EINVAL` | Invalid argument | Bad parameter values |
| `-ENOMEM` | Out of memory | Allocation failed |
| `-EBUSY` | Device busy | Resource in use |
| `-ENODEV` | No such device | Device not found |
| `-ENOENT` | No such entry | File/entry not found |
| `-EIO` | I/O error | Hardware error |
| `-EFAULT` | Bad address | Invalid user pointer |
| `-EACCES` | Permission denied | Access not allowed |
| `-EAGAIN` | Try again | Temporary condition |
| `-EPROBE_DEFER` | Probe deferred | Resource not ready yet |
| `-ETIMEDOUT` | Timeout | Operation timed out |

### Checking Return Values

```c
int ret;

ret = some_function();
if (ret) {
        pr_err("some_function failed: %d\n", ret);
        return ret;
}

/* Or for clearer negative-only errors */
if (ret < 0) {
        pr_err("failed with error %d\n", ret);
        return ret;
}
```

## Error Pointers (ERR_PTR)

Encode error codes in pointer values:

```c
#include <linux/err.h>

/* Return error as pointer */
void *allocate_thing(void)
{
        void *ptr;

        ptr = kmalloc(size, GFP_KERNEL);
        if (!ptr)
                return ERR_PTR(-ENOMEM);

        return ptr;
}

/* Check for error pointer */
void *ptr = allocate_thing();
if (IS_ERR(ptr)) {
        int err = PTR_ERR(ptr);
        pr_err("allocation failed: %d\n", err);
        return err;
}

/* Safe to use ptr now */
```

### ERR_PTR Functions

```c
/* Create error pointer */
ERR_PTR(-ENOMEM)

/* Check if pointer is error */
IS_ERR(ptr)          /* true if error */
IS_ERR_OR_NULL(ptr)  /* true if error or NULL */

/* Extract error code */
PTR_ERR(ptr)         /* Returns negative errno */

/* Return error or valid pointer */
ERR_CAST(ptr)        /* Cast to another pointer type */
```

### Common Pattern

```c
struct resource *get_resource(void)
{
        struct resource *res;

        res = find_resource();
        if (!res)
                return ERR_PTR(-ENOENT);

        if (resource_busy(res))
                return ERR_PTR(-EBUSY);

        return res;
}

/* Caller */
struct resource *res = get_resource();
if (IS_ERR(res))
        return PTR_ERR(res);
```

## The Goto Cleanup Pattern

Standard pattern for handling multiple allocations/registrations:

### Basic Pattern

```c
static int __init my_init(void)
{
        int ret;

        /* Step 1 */
        ret = alloc_resource_a();
        if (ret)
                return ret;

        /* Step 2 */
        ret = alloc_resource_b();
        if (ret)
                goto err_b;

        /* Step 3 */
        ret = register_device();
        if (ret)
                goto err_register;

        return 0;

err_register:
        free_resource_b();
err_b:
        free_resource_a();
        return ret;
}

static void __exit my_exit(void)
{
        /* Cleanup in reverse order */
        unregister_device();
        free_resource_b();
        free_resource_a();
}
```

### Real-World Example

```c
static int my_probe(struct platform_device *pdev)
{
        struct my_device *dev;
        int ret;

        /* Allocate device structure */
        dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
        if (!dev)
                return -ENOMEM;

        /* Get resources */
        dev->base = devm_platform_ioremap_resource(pdev, 0);
        if (IS_ERR(dev->base))
                return PTR_ERR(dev->base);

        dev->irq = platform_get_irq(pdev, 0);
        if (dev->irq < 0)
                return dev->irq;

        /* Request IRQ (not devm, needs manual cleanup) */
        ret = request_irq(dev->irq, my_irq_handler, 0, "my_device", dev);
        if (ret)
                return ret;

        /* Register with subsystem */
        ret = register_my_device(dev);
        if (ret)
                goto err_irq;

        platform_set_drvdata(pdev, dev);
        return 0;

err_irq:
        free_irq(dev->irq, dev);
        return ret;
}

static int my_remove(struct platform_device *pdev)
{
        struct my_device *dev = platform_get_drvdata(pdev);

        unregister_my_device(dev);
        free_irq(dev->irq, dev);
        /* devm resources freed automatically */

        return 0;
}
```

## Managed Resources (devm_*)

The `devm_*` functions automatically free resources when the device is removed:

```c
/* Automatically freed on device removal */
ptr = devm_kzalloc(dev, size, GFP_KERNEL);
base = devm_ioremap_resource(dev, res);
ret = devm_request_irq(dev, irq, handler, flags, name, data);
clk = devm_clk_get(dev, "my_clock");
```

### Benefits

- No need for explicit cleanup code
- Can't forget to free resources
- Cleaner probe/remove functions

### Example: Before and After devm

```c
/* Without devm - manual cleanup */
static int probe(struct device *dev)
{
        void *buf = kmalloc(1024, GFP_KERNEL);
        if (!buf)
                return -ENOMEM;

        void __iomem *base = ioremap(addr, size);
        if (!base) {
                kfree(buf);
                return -ENOMEM;
        }

        int ret = request_irq(irq, handler, 0, "name", dev);
        if (ret) {
                iounmap(base);
                kfree(buf);
                return ret;
        }

        return 0;
}

static void remove(struct device *dev)
{
        free_irq(irq, dev);
        iounmap(base);
        kfree(buf);
}

/* With devm - automatic cleanup */
static int probe(struct device *dev)
{
        void *buf = devm_kmalloc(dev, 1024, GFP_KERNEL);
        if (!buf)
                return -ENOMEM;

        void __iomem *base = devm_ioremap(dev, addr, size);
        if (!base)
                return -ENOMEM;

        int ret = devm_request_irq(dev, irq, handler, 0, "name", dev);
        if (ret)
                return ret;

        return 0;
}

static void remove(struct device *dev)
{
        /* Nothing to do! devm handles it */
}
```

## Logging Errors

Use appropriate log levels:

```c
/* For errors */
pr_err("Failed to allocate memory\n");
dev_err(dev, "Hardware error: %d\n", status);

/* For warnings */
pr_warn("Unexpected condition\n");
dev_warn(dev, "Timeout, retrying\n");

/* Include error code in message */
dev_err(dev, "probe failed: %d\n", ret);
```

### Device-Aware Logging

Prefer `dev_*` over `pr_*` in drivers:

```c
/* Includes device name in output */
dev_info(dev, "Device initialized\n");
/* [    1.234567] my_device 0000:00:01.0: Device initialized */

dev_err(dev, "I/O error on read\n");
/* [    1.234567] my_device 0000:00:01.0: I/O error on read */
```

## Assertions and BUG()

For conditions that should never happen:

```c
/* Print warning but continue */
WARN_ON(condition);
WARN_ON_ONCE(condition);  /* Only warn once */

/* Warning with message */
WARN(condition, "message: %d\n", value);

/* Crash the kernel (use sparingly!) */
BUG_ON(condition);
BUG();
```

{: .warning }
`BUG()` kills the system. Only use for unrecoverable corruption. Prefer `WARN_ON()` and return an error.

## Summary

- Return **negative errno** values for errors
- Use **ERR_PTR/IS_ERR/PTR_ERR** for pointer-returning functions
- Use **goto cleanup** pattern for multiple resources
- Use **devm_*** functions for automatic cleanup
- Log errors with appropriate levels
- Include error codes in log messages

## Next

Learn about [common kernel API macros]({% link part2/06-kernel-api.md %}) and helper functions.
