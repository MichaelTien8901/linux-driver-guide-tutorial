// SPDX-License-Identifier: GPL-2.0
/*
 * sysfs Attribute Demo
 *
 * Demonstrates sysfs device attributes:
 * - Read-only attribute (status)
 * - Read-write attribute (value)
 * - Attribute groups
 */

#include <linux/module.h>
#include <linux/platform_device.h>

struct sysfs_demo {
    int value;
    int access_count;
};

/* Read-only: show status */
static ssize_t status_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    struct sysfs_demo *demo = dev_get_drvdata(dev);

    demo->access_count++;
    return sysfs_emit(buf, "value=%d accesses=%d\n",
                      demo->value, demo->access_count);
}
static DEVICE_ATTR_RO(status);

/* Read-write: get/set value */
static ssize_t value_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct sysfs_demo *demo = dev_get_drvdata(dev);
    return sysfs_emit(buf, "%d\n", demo->value);
}

static ssize_t value_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    struct sysfs_demo *demo = dev_get_drvdata(dev);
    int ret;

    ret = kstrtoint(buf, 0, &demo->value);
    if (ret)
        return ret;

    dev_info(dev, "Value set to %d\n", demo->value);
    return count;
}
static DEVICE_ATTR_RW(value);

/* Attribute group */
static struct attribute *sysfs_demo_attrs[] = {
    &dev_attr_status.attr,
    &dev_attr_value.attr,
    NULL,
};
ATTRIBUTE_GROUPS(sysfs_demo);

/* Platform driver */
static int sysfs_demo_probe(struct platform_device *pdev)
{
    struct sysfs_demo *demo;

    demo = devm_kzalloc(&pdev->dev, sizeof(*demo), GFP_KERNEL);
    if (!demo)
        return -ENOMEM;

    demo->value = 42;
    platform_set_drvdata(pdev, demo);

    dev_info(&pdev->dev, "sysfs demo loaded\n");
    return 0;
}

static struct platform_driver sysfs_demo_driver = {
    .probe = sysfs_demo_probe,
    .driver = {
        .name = "sysfs_demo",
        .dev_groups = sysfs_demo_groups,
    },
};

static struct platform_device *sysfs_demo_pdev;

static int __init sysfs_demo_init(void)
{
    int ret;

    ret = platform_driver_register(&sysfs_demo_driver);
    if (ret)
        return ret;

    sysfs_demo_pdev = platform_device_register_simple("sysfs_demo", -1, NULL, 0);
    if (IS_ERR(sysfs_demo_pdev)) {
        platform_driver_unregister(&sysfs_demo_driver);
        return PTR_ERR(sysfs_demo_pdev);
    }

    return 0;
}

static void __exit sysfs_demo_exit(void)
{
    platform_device_unregister(sysfs_demo_pdev);
    platform_driver_unregister(&sysfs_demo_driver);
}

module_init(sysfs_demo_init);
module_exit(sysfs_demo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sysfs Attribute Demo");
