// SPDX-License-Identifier: GPL-2.0
/*
 * RTC Demo Driver
 *
 * Demonstrates implementing an RTC device with alarm support.
 * Uses software-maintained time with kernel timer for alarm.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/timer.h>
#include <linux/slab.h>

#define DRIVER_NAME     "demo-rtc"

struct demo_rtc {
    struct rtc_device *rtc;
    struct mutex lock;

    /* Software-maintained time */
    time64_t base_time;         /* Time when driver loaded */
    ktime_t base_ktime;         /* ktime when driver loaded */

    /* Alarm */
    struct timer_list alarm_timer;
    time64_t alarm_time;
    bool alarm_enabled;
    bool alarm_pending;
};

/* Calculate current time */
static time64_t demo_rtc_get_time64(struct demo_rtc *drtc)
{
    ktime_t now = ktime_get_real();
    s64 elapsed_ns = ktime_to_ns(ktime_sub(now, drtc->base_ktime));

    return drtc->base_time + div_s64(elapsed_ns, NSEC_PER_SEC);
}

static int demo_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
    struct demo_rtc *drtc = dev_get_drvdata(dev);
    time64_t time;

    mutex_lock(&drtc->lock);
    time = demo_rtc_get_time64(drtc);
    mutex_unlock(&drtc->lock);

    rtc_time64_to_tm(time, tm);

    return 0;
}

static int demo_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
    struct demo_rtc *drtc = dev_get_drvdata(dev);
    time64_t new_time;

    new_time = rtc_tm_to_time64(tm);

    mutex_lock(&drtc->lock);
    drtc->base_time = new_time;
    drtc->base_ktime = ktime_get_real();
    mutex_unlock(&drtc->lock);

    dev_info(dev, "Time set to %ptR\n", tm);

    return 0;
}

static void demo_rtc_alarm_callback(struct timer_list *t)
{
    struct demo_rtc *drtc = from_timer(drtc, t, alarm_timer);

    drtc->alarm_pending = true;

    /* Notify RTC subsystem */
    rtc_update_irq(drtc->rtc, 1, RTC_AF | RTC_IRQF);
}

static int demo_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    struct demo_rtc *drtc = dev_get_drvdata(dev);

    mutex_lock(&drtc->lock);
    rtc_time64_to_tm(drtc->alarm_time, &alrm->time);
    alrm->enabled = drtc->alarm_enabled;
    alrm->pending = drtc->alarm_pending;
    mutex_unlock(&drtc->lock);

    return 0;
}

static int demo_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    struct demo_rtc *drtc = dev_get_drvdata(dev);
    time64_t alarm_time, current_time;
    unsigned long delay_jiffies;

    alarm_time = rtc_tm_to_time64(&alrm->time);

    mutex_lock(&drtc->lock);

    /* Cancel any existing alarm */
    del_timer_sync(&drtc->alarm_timer);
    drtc->alarm_pending = false;

    drtc->alarm_time = alarm_time;
    drtc->alarm_enabled = alrm->enabled;

    if (alrm->enabled) {
        current_time = demo_rtc_get_time64(drtc);

        if (alarm_time > current_time) {
            /* Schedule alarm */
            delay_jiffies = (alarm_time - current_time) * HZ;
            mod_timer(&drtc->alarm_timer, jiffies + delay_jiffies);
            dev_info(dev, "Alarm set for %ptR (in %lld seconds)\n",
                     &alrm->time, alarm_time - current_time);
        } else {
            dev_warn(dev, "Alarm time is in the past\n");
        }
    }

    mutex_unlock(&drtc->lock);

    return 0;
}

static int demo_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
    struct demo_rtc *drtc = dev_get_drvdata(dev);

    mutex_lock(&drtc->lock);

    if (enabled && !drtc->alarm_enabled) {
        time64_t current_time = demo_rtc_get_time64(drtc);

        if (drtc->alarm_time > current_time) {
            unsigned long delay = (drtc->alarm_time - current_time) * HZ;
            mod_timer(&drtc->alarm_timer, jiffies + delay);
        }
    } else if (!enabled) {
        del_timer_sync(&drtc->alarm_timer);
    }

    drtc->alarm_enabled = enabled;
    mutex_unlock(&drtc->lock);

    return 0;
}

static int demo_rtc_read_offset(struct device *dev, long *offset)
{
    /* Demo driver has no offset correction */
    *offset = 0;
    return 0;
}

static const struct rtc_class_ops demo_rtc_ops = {
    .read_time = demo_rtc_read_time,
    .set_time = demo_rtc_set_time,
    .read_alarm = demo_rtc_read_alarm,
    .set_alarm = demo_rtc_set_alarm,
    .alarm_irq_enable = demo_rtc_alarm_irq_enable,
    .read_offset = demo_rtc_read_offset,
};

static int demo_rtc_probe(struct platform_device *pdev)
{
    struct demo_rtc *drtc;
    int ret;

    drtc = devm_kzalloc(&pdev->dev, sizeof(*drtc), GFP_KERNEL);
    if (!drtc)
        return -ENOMEM;

    mutex_init(&drtc->lock);

    /* Initialize software time to current system time */
    drtc->base_ktime = ktime_get_real();
    drtc->base_time = ktime_to_timespec64(drtc->base_ktime).tv_sec;

    /* Initialize alarm timer */
    timer_setup(&drtc->alarm_timer, demo_rtc_alarm_callback, 0);

    platform_set_drvdata(pdev, drtc);

    /* Register RTC device */
    drtc->rtc = devm_rtc_allocate_device(&pdev->dev);
    if (IS_ERR(drtc->rtc))
        return PTR_ERR(drtc->rtc);

    drtc->rtc->ops = &demo_rtc_ops;

    /* Set RTC range (Unix time) */
    drtc->rtc->range_min = 0;
    drtc->rtc->range_max = U64_MAX;

    ret = devm_rtc_register_device(drtc->rtc);
    if (ret)
        return ret;

    dev_info(&pdev->dev, "Demo RTC registered\n");

    return 0;
}

static void demo_rtc_remove(struct platform_device *pdev)
{
    struct demo_rtc *drtc = platform_get_drvdata(pdev);

    del_timer_sync(&drtc->alarm_timer);
}

static const struct of_device_id demo_rtc_of_match[] = {
    { .compatible = "demo,rtc" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_rtc_of_match);

static struct platform_driver demo_rtc_driver = {
    .probe = demo_rtc_probe,
    .remove = demo_rtc_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = demo_rtc_of_match,
    },
};

/* Self-registering platform device for demo */
static struct platform_device *demo_pdev;

static int __init demo_rtc_init(void)
{
    int ret;

    ret = platform_driver_register(&demo_rtc_driver);
    if (ret)
        return ret;

    demo_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(demo_pdev)) {
        platform_driver_unregister(&demo_rtc_driver);
        return PTR_ERR(demo_pdev);
    }

    pr_info("Demo RTC driver loaded\n");
    return 0;
}

static void __exit demo_rtc_exit(void)
{
    platform_device_unregister(demo_pdev);
    platform_driver_unregister(&demo_rtc_driver);
    pr_info("Demo RTC driver unloaded\n");
}

module_init(demo_rtc_init);
module_exit(demo_rtc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Demo RTC Driver with Alarm Support");
