---
layout: default
title: "16.2 PM Implementation"
parent: "Part 16: Power Management"
nav_order: 2
---

# PM Implementation

This chapter shows how to implement power management in your driver.

## dev_pm_ops Structure

```c
#include <linux/pm.h>

static int my_suspend(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    /* Save device state */
    /* Disable interrupts */
    /* Stop DMA */

    return 0;
}

static int my_resume(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    /* Restore device state */
    /* Re-enable interrupts */

    return 0;
}

static int my_runtime_suspend(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    /* Disable clocks, power down */
    clk_disable_unprepare(mydev->clk);

    return 0;
}

static int my_runtime_resume(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    /* Enable clocks, power up */
    clk_prepare_enable(mydev->clk);

    return 0;
}

static const struct dev_pm_ops my_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(my_suspend, my_resume)
    SET_RUNTIME_PM_OPS(my_runtime_suspend, my_runtime_resume, NULL)
};
```

## Registering PM Ops

```c
static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my_device",
        .pm = &my_pm_ops,  /* Add PM ops here */
    },
};
```

## Runtime PM Setup in Probe

```c
static int my_probe(struct platform_device *pdev)
{
    struct my_dev *dev;

    /* ... allocate and initialize ... */

    /* Enable runtime PM */
    pm_runtime_enable(&pdev->dev);

    /* Optional: set autosuspend delay */
    pm_runtime_set_autosuspend_delay(&pdev->dev, 1000);
    pm_runtime_use_autosuspend(&pdev->dev);

    /* Mark as active initially */
    pm_runtime_get_noresume(&pdev->dev);
    pm_runtime_set_active(&pdev->dev);
    pm_runtime_put(&pdev->dev);  /* Allow suspend when idle */

    return 0;
}

static void my_remove(struct platform_device *pdev)
{
    /* Disable runtime PM before removing */
    pm_runtime_disable(&pdev->dev);
}
```

## Using Runtime PM in Your Code

```c
static int my_read_data(struct my_dev *dev)
{
    int ret, data;

    /* Ensure device is active before accessing hardware */
    ret = pm_runtime_get_sync(dev->dev);
    if (ret < 0) {
        pm_runtime_put_noidle(dev->dev);
        return ret;
    }

    /* Now safe to access hardware */
    data = readl(dev->regs + DATA_REG);

    /* Mark as busy and allow suspend after delay */
    pm_runtime_mark_last_busy(dev->dev);
    pm_runtime_put_autosuspend(dev->dev);

    return data;
}
```

## Common Patterns

### Pattern 1: Simple Suspend/Resume

```c
static int my_runtime_suspend(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    clk_disable_unprepare(mydev->clk);
    return 0;
}

static int my_runtime_resume(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    return clk_prepare_enable(mydev->clk);
}
```

### Pattern 2: System Sleep Uses Runtime PM

```c
static int my_suspend(struct device *dev)
{
    /* Use runtime PM to do the actual work */
    return pm_runtime_force_suspend(dev);
}

static int my_resume(struct device *dev)
{
    return pm_runtime_force_resume(dev);
}

static const struct dev_pm_ops my_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(my_suspend, my_resume)
    SET_RUNTIME_PM_OPS(my_runtime_suspend, my_runtime_resume, NULL)
};
```

### Pattern 3: IRQ Wake Source

```c
static int my_suspend(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    /* Enable wake from this device */
    if (device_may_wakeup(dev))
        enable_irq_wake(mydev->irq);

    return 0;
}

static int my_resume(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    if (device_may_wakeup(dev))
        disable_irq_wake(mydev->irq);

    return 0;
}

/* In probe */
device_init_wakeup(&pdev->dev, true);
```

## PM Helper Macros

| Macro | Creates |
|-------|---------|
| `SET_SYSTEM_SLEEP_PM_OPS(s, r)` | `.suspend`, `.resume`, `.freeze`, `.thaw` |
| `SET_RUNTIME_PM_OPS(s, r, i)` | `.runtime_suspend`, `.runtime_resume`, `.runtime_idle` |
| `DEFINE_SIMPLE_DEV_PM_OPS(name, s, r)` | Complete pm_ops for simple case |

## Debug Runtime PM

```bash
# Check PM state
cat /sys/devices/.../power/runtime_status

# Force suspend (for testing)
echo auto > /sys/devices/.../power/control

# Disable runtime PM
echo on > /sys/devices/.../power/control
```

## Summary

| Step | What to Do |
|------|------------|
| Define ops | Create `dev_pm_ops` with callbacks |
| Register | Set `.pm` in driver |
| Enable | `pm_runtime_enable()` in probe |
| Use | `pm_runtime_get/put()` around hw access |
| Disable | `pm_runtime_disable()` in remove |

## Further Reading

- [Runtime PM API](https://docs.kernel.org/power/runtime_pm.html) - Complete reference
- [Device PM](https://docs.kernel.org/driver-api/pm/devices.html) - Guidelines
