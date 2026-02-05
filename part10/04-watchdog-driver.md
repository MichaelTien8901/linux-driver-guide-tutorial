---
layout: default
title: "10.4 Watchdog Driver"
parent: "Part 10: PWM, Watchdog, HWMON, LED Drivers"
nav_order: 4
---

# Watchdog Driver

This chapter covers implementing a watchdog timer driver using the `watchdog_device` structure.

## Basic Watchdog Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/io.h>
#include <linux/clk.h>

#define REG_CTRL        0x00
#define REG_LOAD        0x04
#define REG_COUNT       0x08
#define REG_LOCK        0x0C

#define CTRL_ENABLE     BIT(0)
#define CTRL_RESET_EN   BIT(1)
#define CTRL_INT_EN     BIT(2)

#define WDT_UNLOCK_KEY  0x1ACCE551
#define WDT_LOCK_KEY    0x00000001

#define WDT_MIN_TIMEOUT 1
#define WDT_MAX_TIMEOUT 60
#define WDT_DEFAULT_TIMEOUT 30

struct my_wdt {
    struct watchdog_device wdd;
    void __iomem *regs;
    struct clk *clk;
    unsigned long clk_rate;
    spinlock_t lock;
};

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0444);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started");

static inline void wdt_unlock(struct my_wdt *wdt)
{
    writel(WDT_UNLOCK_KEY, wdt->regs + REG_LOCK);
}

static inline void wdt_lock(struct my_wdt *wdt)
{
    writel(WDT_LOCK_KEY, wdt->regs + REG_LOCK);
}

static int my_wdt_start(struct watchdog_device *wdd)
{
    struct my_wdt *wdt = watchdog_get_drvdata(wdd);
    unsigned long flags;
    u32 load_value;

    load_value = wdd->timeout * wdt->clk_rate;

    spin_lock_irqsave(&wdt->lock, flags);

    wdt_unlock(wdt);

    /* Set timeout value */
    writel(load_value, wdt->regs + REG_LOAD);

    /* Enable watchdog with reset */
    writel(CTRL_ENABLE | CTRL_RESET_EN, wdt->regs + REG_CTRL);

    wdt_lock(wdt);

    spin_unlock_irqrestore(&wdt->lock, flags);

    return 0;
}

static int my_wdt_stop(struct watchdog_device *wdd)
{
    struct my_wdt *wdt = watchdog_get_drvdata(wdd);
    unsigned long flags;

    spin_lock_irqsave(&wdt->lock, flags);

    wdt_unlock(wdt);
    writel(0, wdt->regs + REG_CTRL);
    wdt_lock(wdt);

    spin_unlock_irqrestore(&wdt->lock, flags);

    return 0;
}

static int my_wdt_ping(struct watchdog_device *wdd)
{
    struct my_wdt *wdt = watchdog_get_drvdata(wdd);
    unsigned long flags;
    u32 load_value;

    load_value = wdd->timeout * wdt->clk_rate;

    spin_lock_irqsave(&wdt->lock, flags);

    wdt_unlock(wdt);
    writel(load_value, wdt->regs + REG_LOAD);
    wdt_lock(wdt);

    spin_unlock_irqrestore(&wdt->lock, flags);

    return 0;
}

static int my_wdt_set_timeout(struct watchdog_device *wdd, unsigned int timeout)
{
    struct my_wdt *wdt = watchdog_get_drvdata(wdd);

    wdd->timeout = timeout;

    /* If running, update the hardware */
    if (watchdog_active(wdd))
        my_wdt_ping(wdd);

    return 0;
}

static unsigned int my_wdt_get_timeleft(struct watchdog_device *wdd)
{
    struct my_wdt *wdt = watchdog_get_drvdata(wdd);
    u32 count;

    count = readl(wdt->regs + REG_COUNT);

    return count / wdt->clk_rate;
}

static const struct watchdog_ops my_wdt_ops = {
    .owner = THIS_MODULE,
    .start = my_wdt_start,
    .stop = my_wdt_stop,
    .ping = my_wdt_ping,
    .set_timeout = my_wdt_set_timeout,
    .get_timeleft = my_wdt_get_timeleft,
};

static const struct watchdog_info my_wdt_info = {
    .identity = "My Watchdog Timer",
    .options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
};

static int my_wdt_probe(struct platform_device *pdev)
{
    struct my_wdt *wdt;
    int ret;

    wdt = devm_kzalloc(&pdev->dev, sizeof(*wdt), GFP_KERNEL);
    if (!wdt)
        return -ENOMEM;

    wdt->regs = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(wdt->regs))
        return PTR_ERR(wdt->regs);

    wdt->clk = devm_clk_get(&pdev->dev, NULL);
    if (IS_ERR(wdt->clk))
        return dev_err_probe(&pdev->dev, PTR_ERR(wdt->clk),
                             "Failed to get clock\n");

    ret = clk_prepare_enable(wdt->clk);
    if (ret)
        return ret;

    wdt->clk_rate = clk_get_rate(wdt->clk);
    if (!wdt->clk_rate) {
        clk_disable_unprepare(wdt->clk);
        return -EINVAL;
    }

    spin_lock_init(&wdt->lock);

    /* Configure watchdog_device */
    wdt->wdd.info = &my_wdt_info;
    wdt->wdd.ops = &my_wdt_ops;
    wdt->wdd.parent = &pdev->dev;
    wdt->wdd.min_timeout = WDT_MIN_TIMEOUT;
    wdt->wdd.max_timeout = WDT_MAX_TIMEOUT;
    wdt->wdd.timeout = WDT_DEFAULT_TIMEOUT;

    /* Read timeout from device tree if available */
    watchdog_init_timeout(&wdt->wdd, 0, &pdev->dev);

    /* Set nowayout */
    watchdog_set_nowayout(&wdt->wdd, nowayout);

    /* Store driver data */
    watchdog_set_drvdata(&wdt->wdd, wdt);

    /* Stop watchdog on reboot */
    watchdog_stop_on_reboot(&wdt->wdd);

    /* Stop watchdog on unregister */
    watchdog_stop_on_unregister(&wdt->wdd);

    /* Register watchdog device */
    ret = devm_watchdog_register_device(&pdev->dev, &wdt->wdd);
    if (ret) {
        clk_disable_unprepare(wdt->clk);
        return dev_err_probe(&pdev->dev, ret,
                             "Failed to register watchdog\n");
    }

    platform_set_drvdata(pdev, wdt);

    dev_info(&pdev->dev, "Watchdog registered: timeout=%d sec, nowayout=%d\n",
             wdt->wdd.timeout, nowayout);

    return 0;
}

static void my_wdt_remove(struct platform_device *pdev)
{
    struct my_wdt *wdt = platform_get_drvdata(pdev);

    clk_disable_unprepare(wdt->clk);
}

static const struct of_device_id my_wdt_of_match[] = {
    { .compatible = "vendor,my-watchdog" },
    { }
};
MODULE_DEVICE_TABLE(of, my_wdt_of_match);

static struct platform_driver my_wdt_driver = {
    .probe = my_wdt_probe,
    .remove = my_wdt_remove,
    .driver = {
        .name = "my-watchdog",
        .of_match_table = my_wdt_of_match,
    },
};
module_platform_driver(my_wdt_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Example Watchdog Driver");
```

## Watchdog with Pretimeout

```c
#include <linux/interrupt.h>

struct my_wdt_pretimeout {
    struct watchdog_device wdd;
    void __iomem *regs;
    int pretimeout_irq;
    struct clk *clk;
    unsigned long clk_rate;
};

static irqreturn_t my_wdt_pretimeout_isr(int irq, void *data)
{
    struct my_wdt_pretimeout *wdt = data;

    /* Clear interrupt */
    writel(INT_CLEAR, wdt->regs + REG_INT_STATUS);

    /* Notify watchdog core - triggers pretimeout governor action */
    watchdog_notify_pretimeout(&wdt->wdd);

    return IRQ_HANDLED;
}

static int my_wdt_set_pretimeout(struct watchdog_device *wdd,
                                 unsigned int pretimeout)
{
    struct my_wdt_pretimeout *wdt = watchdog_get_drvdata(wdd);
    u32 pretimeout_load;

    if (pretimeout > wdd->timeout)
        return -EINVAL;

    wdd->pretimeout = pretimeout;

    if (pretimeout) {
        /* Set pretimeout value */
        pretimeout_load = pretimeout * wdt->clk_rate;
        writel(pretimeout_load, wdt->regs + REG_PRETIMEOUT);

        /* Enable pretimeout interrupt */
        writel(CTRL_INT_EN, wdt->regs + REG_CTRL);
    } else {
        /* Disable pretimeout interrupt */
        writel(0, wdt->regs + REG_CTRL);
    }

    return 0;
}

static const struct watchdog_ops my_wdt_pretimeout_ops = {
    .owner = THIS_MODULE,
    .start = my_wdt_start,
    .stop = my_wdt_stop,
    .ping = my_wdt_ping,
    .set_timeout = my_wdt_set_timeout,
    .set_pretimeout = my_wdt_set_pretimeout,
    .get_timeleft = my_wdt_get_timeleft,
};

static const struct watchdog_info my_wdt_pretimeout_info = {
    .identity = "My Watchdog with Pretimeout",
    .options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING |
               WDIOF_MAGICCLOSE | WDIOF_PRETIMEOUT,
};

static int my_wdt_pretimeout_probe(struct platform_device *pdev)
{
    struct my_wdt_pretimeout *wdt;
    int ret;

    wdt = devm_kzalloc(&pdev->dev, sizeof(*wdt), GFP_KERNEL);
    if (!wdt)
        return -ENOMEM;

    /* ... resource setup ... */

    wdt->pretimeout_irq = platform_get_irq(pdev, 0);
    if (wdt->pretimeout_irq < 0)
        return wdt->pretimeout_irq;

    ret = devm_request_irq(&pdev->dev, wdt->pretimeout_irq,
                           my_wdt_pretimeout_isr, 0,
                           "watchdog-pretimeout", wdt);
    if (ret)
        return ret;

    wdt->wdd.info = &my_wdt_pretimeout_info;
    wdt->wdd.ops = &my_wdt_pretimeout_ops;
    /* ... rest of setup ... */

    return devm_watchdog_register_device(&pdev->dev, &wdt->wdd);
}
```

## Watchdog as Restart Handler

```c
static int my_wdt_restart(struct watchdog_device *wdd,
                          unsigned long action, void *data)
{
    struct my_wdt *wdt = watchdog_get_drvdata(wdd);

    /* Set minimal timeout to force immediate reset */
    writel(1, wdt->regs + REG_LOAD);
    writel(CTRL_ENABLE | CTRL_RESET_EN, wdt->regs + REG_CTRL);

    /* Wait for reset */
    mdelay(500);

    return 0;
}

static const struct watchdog_ops my_wdt_restart_ops = {
    .owner = THIS_MODULE,
    .start = my_wdt_start,
    .stop = my_wdt_stop,
    .ping = my_wdt_ping,
    .set_timeout = my_wdt_set_timeout,
    .restart = my_wdt_restart,  /* Enables restart handler */
};

/* In probe */
/* Set restart priority (higher = preferred) */
wdt->wdd.restart_nb.priority = 128;
```

## Hardware Running at Boot

Some watchdogs are already running when the driver probes:

```c
static int my_wdt_probe(struct platform_device *pdev)
{
    /* Check if hardware watchdog is running */
    u32 ctrl = readl(wdt->regs + REG_CTRL);
    if (ctrl & CTRL_ENABLE) {
        /* Tell core the hardware is already running */
        set_bit(WDOG_HW_RUNNING, &wdt->wdd.status);

        /* Read current timeout from hardware */
        u32 load = readl(wdt->regs + REG_LOAD);
        wdt->wdd.timeout = load / wdt->clk_rate;
    }

    /* ... register device ... */
}
```

## Handling Bootstatus

Report why the system rebooted:

```c
static int my_wdt_probe(struct platform_device *pdev)
{
    u32 status = readl(wdt->regs + REG_RESET_STATUS);

    if (status & RESET_WDT)
        wdt->wdd.bootstatus = WDIOF_CARDRESET;
    else if (status & RESET_OVERHEAT)
        wdt->wdd.bootstatus = WDIOF_OVERHEAT;
    else
        wdt->wdd.bootstatus = 0;

    /* Clear reset status */
    writel(status, wdt->regs + REG_RESET_STATUS);

    /* ... */
}
```

## Device Tree Binding

```dts
watchdog@10000000 {
    compatible = "vendor,my-watchdog";
    reg = <0x10000000 0x100>;
    clocks = <&wdt_clk>;
    clock-names = "wdt";
    timeout-sec = <30>;
    /* Optional pretimeout interrupt */
    interrupts = <GIC_SPI 45 IRQ_TYPE_LEVEL_HIGH>;
};
```

## Power Management

```c
static int my_wdt_suspend(struct device *dev)
{
    struct my_wdt *wdt = dev_get_drvdata(dev);

    if (watchdog_active(&wdt->wdd))
        my_wdt_stop(&wdt->wdd);

    clk_disable_unprepare(wdt->clk);

    return 0;
}

static int my_wdt_resume(struct device *dev)
{
    struct my_wdt *wdt = dev_get_drvdata(dev);
    int ret;

    ret = clk_prepare_enable(wdt->clk);
    if (ret)
        return ret;

    if (watchdog_active(&wdt->wdd))
        my_wdt_start(&wdt->wdd);

    return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(my_wdt_pm_ops,
                                my_wdt_suspend, my_wdt_resume);
```

## Summary

- Implement `start()`, `stop()`, `ping()` at minimum
- Use `devm_watchdog_register_device()` for automatic cleanup
- Set `nowayout` to prevent stopping once started
- Use `watchdog_init_timeout()` to read DT timeout
- Call `watchdog_stop_on_reboot()` for clean shutdown
- Implement `restart()` to use watchdog for system reset
- Report `bootstatus` for debugging boot failures

## Further Reading

- [Watchdog Timer Driver API](https://docs.kernel.org/watchdog/watchdog-kernel-api.html) - Kernel API
- [Watchdog Drivers](https://elixir.bootlin.com/linux/v6.6/source/drivers/watchdog) - Example drivers
- [Device Tree Bindings](https://docs.kernel.org/devicetree/bindings/watchdog/watchdog.yaml) - DT schema

## Next

Learn about the [HWMON subsystem]({% link part10/05-hwmon-subsystem.md %}).
