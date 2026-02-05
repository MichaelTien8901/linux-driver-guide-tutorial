// SPDX-License-Identifier: GPL-2.0
/*
 * Power Management Demo Driver
 *
 * Demonstrates both runtime PM and system sleep PM:
 * - pm_runtime_get/put reference counting
 * - Autosuspend with delay
 * - System suspend/resume callbacks
 * - Proper PM initialization sequence
 *
 * Creates /sys/devices/platform/pm_demo.0/data for testing.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>

#define DRIVER_NAME "pm_demo"
#define AUTOSUSPEND_DELAY_MS 2000  /* 2 seconds */

struct pm_demo_dev {
    struct device *dev;
    bool powered;
    int access_count;

    /* Simulated hardware state */
    u32 saved_data;
};

/* ============ Simulated Hardware Access ============ */

static int pm_demo_hw_read(struct pm_demo_dev *pmdev)
{
    /* Ensure device is powered before access */
    int ret = pm_runtime_get_sync(pmdev->dev);
    if (ret < 0) {
        pm_runtime_put_noidle(pmdev->dev);
        return ret;
    }

    pmdev->access_count++;
    dev_info(pmdev->dev, "Hardware read (access #%d)\n", pmdev->access_count);

    /* Mark as busy and schedule autosuspend */
    pm_runtime_mark_last_busy(pmdev->dev);
    pm_runtime_put_autosuspend(pmdev->dev);

    return pmdev->saved_data;
}

static int pm_demo_hw_write(struct pm_demo_dev *pmdev, u32 value)
{
    int ret = pm_runtime_get_sync(pmdev->dev);
    if (ret < 0) {
        pm_runtime_put_noidle(pmdev->dev);
        return ret;
    }

    pmdev->saved_data = value;
    pmdev->access_count++;
    dev_info(pmdev->dev, "Hardware write: %u (access #%d)\n",
             value, pmdev->access_count);

    pm_runtime_mark_last_busy(pmdev->dev);
    pm_runtime_put_autosuspend(pmdev->dev);

    return 0;
}

/* ============ sysfs Interface for Testing ============ */

static ssize_t data_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct pm_demo_dev *pmdev = dev_get_drvdata(dev);
    int val = pm_demo_hw_read(pmdev);

    if (val < 0)
        return val;

    return sysfs_emit(buf, "%d\n", val);
}

static ssize_t data_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t count)
{
    struct pm_demo_dev *pmdev = dev_get_drvdata(dev);
    u32 val;
    int ret;

    ret = kstrtou32(buf, 0, &val);
    if (ret)
        return ret;

    ret = pm_demo_hw_write(pmdev, val);
    if (ret)
        return ret;

    return count;
}
static DEVICE_ATTR_RW(data);

static ssize_t status_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    struct pm_demo_dev *pmdev = dev_get_drvdata(dev);

    return sysfs_emit(buf, "powered: %s\naccesses: %d\nruntime_status: %s\n",
                      pmdev->powered ? "yes" : "no",
                      pmdev->access_count,
                      pm_runtime_active(dev) ? "active" : "suspended");
}
static DEVICE_ATTR_RO(status);

static struct attribute *pm_demo_attrs[] = {
    &dev_attr_data.attr,
    &dev_attr_status.attr,
    NULL,
};
ATTRIBUTE_GROUPS(pm_demo);

/* ============ Runtime PM Callbacks ============ */

static int pm_demo_runtime_suspend(struct device *dev)
{
    struct pm_demo_dev *pmdev = dev_get_drvdata(dev);

    dev_info(dev, "Runtime suspend - powering off\n");

    /* Simulate power down */
    pmdev->powered = false;

    return 0;
}

static int pm_demo_runtime_resume(struct device *dev)
{
    struct pm_demo_dev *pmdev = dev_get_drvdata(dev);

    dev_info(dev, "Runtime resume - powering on\n");

    /* Simulate power up */
    pmdev->powered = true;

    return 0;
}

/* ============ System Sleep Callbacks ============ */

static int pm_demo_suspend(struct device *dev)
{
    struct pm_demo_dev *pmdev = dev_get_drvdata(dev);

    dev_info(dev, "System suspend - saving state (data=%u)\n",
             pmdev->saved_data);

    /* State is already saved in pmdev->saved_data */
    /* In real driver: save registers, disable IRQ, etc. */

    return 0;
}

static int pm_demo_resume(struct device *dev)
{
    struct pm_demo_dev *pmdev = dev_get_drvdata(dev);

    dev_info(dev, "System resume - restoring state (data=%u)\n",
             pmdev->saved_data);

    /* In real driver: restore registers, enable IRQ, etc. */

    return 0;
}

/* ============ PM Ops ============ */

static const struct dev_pm_ops pm_demo_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(pm_demo_suspend, pm_demo_resume)
    SET_RUNTIME_PM_OPS(pm_demo_runtime_suspend, pm_demo_runtime_resume, NULL)
};

/* ============ Probe/Remove ============ */

static int pm_demo_probe(struct platform_device *pdev)
{
    struct pm_demo_dev *pmdev;

    pmdev = devm_kzalloc(&pdev->dev, sizeof(*pmdev), GFP_KERNEL);
    if (!pmdev)
        return -ENOMEM;

    pmdev->dev = &pdev->dev;
    pmdev->powered = true;
    pmdev->saved_data = 42;  /* Initial value */

    platform_set_drvdata(pdev, pmdev);

    /* Enable runtime PM with autosuspend */
    pm_runtime_set_autosuspend_delay(&pdev->dev, AUTOSUSPEND_DELAY_MS);
    pm_runtime_use_autosuspend(&pdev->dev);
    pm_runtime_set_active(&pdev->dev);
    pm_runtime_enable(&pdev->dev);

    dev_info(&pdev->dev,
             "PM demo loaded (autosuspend delay: %d ms)\n",
             AUTOSUSPEND_DELAY_MS);

    return 0;
}

static void pm_demo_remove(struct platform_device *pdev)
{
    pm_runtime_disable(&pdev->dev);
    dev_info(&pdev->dev, "PM demo unloaded\n");
}

/* ============ Platform Driver ============ */

static struct platform_driver pm_demo_driver = {
    .probe = pm_demo_probe,
    .remove = pm_demo_remove,
    .driver = {
        .name = DRIVER_NAME,
        .pm = &pm_demo_pm_ops,
        .dev_groups = pm_demo_groups,
    },
};

/* Self-registering platform device for demo */
static struct platform_device *pm_demo_pdev;

static int __init pm_demo_init(void)
{
    int ret;

    ret = platform_driver_register(&pm_demo_driver);
    if (ret)
        return ret;

    pm_demo_pdev = platform_device_register_simple(DRIVER_NAME, 0, NULL, 0);
    if (IS_ERR(pm_demo_pdev)) {
        platform_driver_unregister(&pm_demo_driver);
        return PTR_ERR(pm_demo_pdev);
    }

    return 0;
}

static void __exit pm_demo_exit(void)
{
    platform_device_unregister(pm_demo_pdev);
    platform_driver_unregister(&pm_demo_driver);
}

module_init(pm_demo_init);
module_exit(pm_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Power Management Demo Driver");
