// SPDX-License-Identifier: GPL-2.0
/*
 * Watchdog Timer Demo Driver
 *
 * Demonstrates implementing a watchdog_device for a virtual watchdog.
 * Creates /dev/watchdog device for userspace interaction.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define DRIVER_NAME         "demo-watchdog"
#define WDT_MIN_TIMEOUT     1
#define WDT_MAX_TIMEOUT     60
#define WDT_DEFAULT_TIMEOUT 30

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0444);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started");

struct demo_wdt {
    struct watchdog_device wdd;
    struct timer_list timer;
    unsigned long last_ping;
    bool expired;
    spinlock_t lock;
};

/* Timer callback - simulates watchdog timeout */
static void demo_wdt_timer_cb(struct timer_list *t)
{
    struct demo_wdt *wdt = from_timer(wdt, t, timer);
    unsigned long flags;

    spin_lock_irqsave(&wdt->lock, flags);

    if (watchdog_active(&wdt->wdd)) {
        unsigned long elapsed = jiffies - wdt->last_ping;
        unsigned long timeout_jiffies = wdt->wdd.timeout * HZ;

        if (elapsed >= timeout_jiffies) {
            pr_crit("demo-watchdog: TIMEOUT! System would reset.\n");
            wdt->expired = true;
            /* In real hardware, this would reset the system */
        } else {
            /* Re-arm timer for next check */
            mod_timer(&wdt->timer, jiffies + HZ);
        }
    }

    spin_unlock_irqrestore(&wdt->lock, flags);
}

static int demo_wdt_start(struct watchdog_device *wdd)
{
    struct demo_wdt *wdt = watchdog_get_drvdata(wdd);
    unsigned long flags;

    spin_lock_irqsave(&wdt->lock, flags);

    wdt->last_ping = jiffies;
    wdt->expired = false;

    /* Start the timer */
    mod_timer(&wdt->timer, jiffies + HZ);

    spin_unlock_irqrestore(&wdt->lock, flags);

    pr_info("demo-watchdog: Started (timeout=%u sec)\n", wdd->timeout);

    return 0;
}

static int demo_wdt_stop(struct watchdog_device *wdd)
{
    struct demo_wdt *wdt = watchdog_get_drvdata(wdd);
    unsigned long flags;

    spin_lock_irqsave(&wdt->lock, flags);

    del_timer_sync(&wdt->timer);
    wdt->expired = false;

    spin_unlock_irqrestore(&wdt->lock, flags);

    pr_info("demo-watchdog: Stopped\n");

    return 0;
}

static int demo_wdt_ping(struct watchdog_device *wdd)
{
    struct demo_wdt *wdt = watchdog_get_drvdata(wdd);
    unsigned long flags;

    spin_lock_irqsave(&wdt->lock, flags);

    wdt->last_ping = jiffies;

    /* Reset timer */
    if (watchdog_active(wdd))
        mod_timer(&wdt->timer, jiffies + HZ);

    spin_unlock_irqrestore(&wdt->lock, flags);

    pr_debug("demo-watchdog: Ping!\n");

    return 0;
}

static int demo_wdt_set_timeout(struct watchdog_device *wdd, unsigned int timeout)
{
    wdd->timeout = timeout;
    pr_info("demo-watchdog: Timeout set to %u sec\n", timeout);

    /* If running, update the timer */
    if (watchdog_active(wdd))
        demo_wdt_ping(wdd);

    return 0;
}

static unsigned int demo_wdt_get_timeleft(struct watchdog_device *wdd)
{
    struct demo_wdt *wdt = watchdog_get_drvdata(wdd);
    unsigned long flags;
    unsigned int timeleft;

    spin_lock_irqsave(&wdt->lock, flags);

    if (watchdog_active(wdd)) {
        unsigned long elapsed = jiffies - wdt->last_ping;
        unsigned long timeout_jiffies = wdd->timeout * HZ;

        if (elapsed >= timeout_jiffies)
            timeleft = 0;
        else
            timeleft = (timeout_jiffies - elapsed) / HZ;
    } else {
        timeleft = wdd->timeout;
    }

    spin_unlock_irqrestore(&wdt->lock, flags);

    return timeleft;
}

static const struct watchdog_ops demo_wdt_ops = {
    .owner = THIS_MODULE,
    .start = demo_wdt_start,
    .stop = demo_wdt_stop,
    .ping = demo_wdt_ping,
    .set_timeout = demo_wdt_set_timeout,
    .get_timeleft = demo_wdt_get_timeleft,
};

static const struct watchdog_info demo_wdt_info = {
    .identity = "Demo Watchdog Timer",
    .options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
};

static int demo_wdt_probe(struct platform_device *pdev)
{
    struct demo_wdt *wdt;
    int ret;

    wdt = devm_kzalloc(&pdev->dev, sizeof(*wdt), GFP_KERNEL);
    if (!wdt)
        return -ENOMEM;

    spin_lock_init(&wdt->lock);
    timer_setup(&wdt->timer, demo_wdt_timer_cb, 0);

    /* Configure watchdog_device */
    wdt->wdd.info = &demo_wdt_info;
    wdt->wdd.ops = &demo_wdt_ops;
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
    if (ret)
        return dev_err_probe(&pdev->dev, ret,
                             "Failed to register watchdog\n");

    platform_set_drvdata(pdev, wdt);

    dev_info(&pdev->dev, "Watchdog registered: timeout=%d sec, nowayout=%d\n",
             wdt->wdd.timeout, nowayout);

    return 0;
}

static void demo_wdt_remove(struct platform_device *pdev)
{
    struct demo_wdt *wdt = platform_get_drvdata(pdev);

    del_timer_sync(&wdt->timer);
    dev_info(&pdev->dev, "Watchdog removed\n");
}

static const struct of_device_id demo_wdt_of_match[] = {
    { .compatible = "demo,watchdog" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_wdt_of_match);

static struct platform_driver demo_wdt_driver = {
    .probe = demo_wdt_probe,
    .remove = demo_wdt_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = demo_wdt_of_match,
    },
};

/* Self-registering platform device */
static struct platform_device *demo_pdev;

static int __init demo_wdt_init(void)
{
    int ret;

    ret = platform_driver_register(&demo_wdt_driver);
    if (ret)
        return ret;

    demo_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(demo_pdev)) {
        platform_driver_unregister(&demo_wdt_driver);
        return PTR_ERR(demo_pdev);
    }

    return 0;
}

static void __exit demo_wdt_exit(void)
{
    platform_device_unregister(demo_pdev);
    platform_driver_unregister(&demo_wdt_driver);
}

module_init(demo_wdt_init);
module_exit(demo_wdt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Demo Watchdog Timer Driver");
