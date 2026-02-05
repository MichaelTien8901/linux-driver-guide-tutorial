---
layout: default
title: "6.6 Device Attributes"
parent: "Part 6: Device Model and Driver Framework"
nav_order: 6
---

# Device Attributes

Device attributes expose device information and configuration through sysfs. They create files under `/sys/` that user space can read and write.

## Basic Device Attribute

```c
#include <linux/device.h>

/* Show function (read) */
static ssize_t my_attr_show(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
    struct my_device *mydev = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", mydev->value);
}

/* Store function (write) */
static ssize_t my_attr_store(struct device *dev,
                             struct device_attribute *attr,
                             const char *buf, size_t count)
{
    struct my_device *mydev = dev_get_drvdata(dev);
    int val;

    if (kstrtoint(buf, 10, &val))
        return -EINVAL;

    mydev->value = val;
    return count;
}

/* Create the attribute */
static DEVICE_ATTR_RW(my_attr);

/* Or using explicit permissions */
static DEVICE_ATTR(my_attr, 0644, my_attr_show, my_attr_store);
```

## Attribute Macros

```c
/* Read-write attribute (0644) */
static DEVICE_ATTR_RW(name);  /* Needs name_show and name_store */

/* Read-only attribute (0444) */
static DEVICE_ATTR_RO(name);  /* Needs name_show */

/* Write-only attribute (0200) */
static DEVICE_ATTR_WO(name);  /* Needs name_store */

/* Admin read-write (0600) */
static DEVICE_ATTR_ADMIN_RW(name);

/* Admin read-only (0400) */
static DEVICE_ATTR_ADMIN_RO(name);
```

## Registering Attributes

### Method 1: Individual Files

```c
static int my_probe(struct platform_device *pdev)
{
    int ret;

    /* ... device setup ... */

    ret = device_create_file(&pdev->dev, &dev_attr_my_attr);
    if (ret)
        return ret;

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    device_remove_file(&pdev->dev, &dev_attr_my_attr);
    return 0;
}
```

### Method 2: Attribute Groups (Recommended)

```c
/* Define attributes */
static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(status);
static DEVICE_ATTR_RW(config);

/* Gather into array */
static struct attribute *my_attrs[] = {
    &dev_attr_value.attr,
    &dev_attr_status.attr,
    &dev_attr_config.attr,
    NULL,
};

/* Create attribute group */
static const struct attribute_group my_group = {
    .attrs = my_attrs,
};

/* Array of groups */
static const struct attribute_group *my_groups[] = {
    &my_group,
    NULL,
};
```

### Using Groups with Platform Driver

```c
static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my_device",
        .dev_groups = my_groups,  /* Auto-created on probe */
    },
};
```

### Using Groups Manually

```c
static int my_probe(struct platform_device *pdev)
{
    int ret;

    ret = sysfs_create_group(&pdev->dev.kobj, &my_group);
    if (ret)
        return ret;

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &my_group);
    return 0;
}
```

## Named Subdirectory

```c
static const struct attribute_group my_group = {
    .name = "config",     /* Creates /sys/.../device/config/ */
    .attrs = my_attrs,
};
```

## Binary Attributes

For binary data (firmware, EEPROM):

```c
static ssize_t eeprom_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count)
{
    struct device *dev = kobj_to_dev(kobj);
    struct my_device *mydev = dev_get_drvdata(dev);

    if (off >= EEPROM_SIZE)
        return 0;

    if (off + count > EEPROM_SIZE)
        count = EEPROM_SIZE - off;

    memcpy(buf, mydev->eeprom + off, count);
    return count;
}

static ssize_t eeprom_write(struct file *filp, struct kobject *kobj,
                            struct bin_attribute *attr,
                            char *buf, loff_t off, size_t count)
{
    struct device *dev = kobj_to_dev(kobj);
    struct my_device *mydev = dev_get_drvdata(dev);

    if (off >= EEPROM_SIZE)
        return -ENOSPC;

    if (off + count > EEPROM_SIZE)
        count = EEPROM_SIZE - off;

    memcpy(mydev->eeprom + off, buf, count);
    return count;
}

static BIN_ATTR_RW(eeprom, EEPROM_SIZE);

static struct bin_attribute *my_bin_attrs[] = {
    &bin_attr_eeprom,
    NULL,
};

static const struct attribute_group my_group = {
    .attrs = my_attrs,
    .bin_attrs = my_bin_attrs,
};
```

## Conditional Attributes

Show attributes only when relevant:

```c
static umode_t my_attr_visible(struct kobject *kobj,
                               struct attribute *attr, int n)
{
    struct device *dev = kobj_to_dev(kobj);
    struct my_device *mydev = dev_get_drvdata(dev);

    /* Hide optional_feature if not supported */
    if (attr == &dev_attr_optional_feature.attr && !mydev->has_feature)
        return 0;  /* Invisible */

    return attr->mode;  /* Use default mode */
}

static const struct attribute_group my_group = {
    .attrs = my_attrs,
    .is_visible = my_attr_visible,
};
```

## Complete Example

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>

struct my_device {
    int value;
    int mode;
    bool enabled;
};

/* Value attribute */
static ssize_t value_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct my_device *mydev = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", mydev->value);
}

static ssize_t value_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    struct my_device *mydev = dev_get_drvdata(dev);
    int val;

    if (kstrtoint(buf, 10, &val))
        return -EINVAL;

    if (val < 0 || val > 100)
        return -EINVAL;

    mydev->value = val;
    return count;
}
static DEVICE_ATTR_RW(value);

/* Mode attribute */
static ssize_t mode_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct my_device *mydev = dev_get_drvdata(dev);
    const char *modes[] = { "off", "low", "medium", "high" };

    if (mydev->mode >= ARRAY_SIZE(modes))
        return -EINVAL;

    return sprintf(buf, "%s\n", modes[mydev->mode]);
}

static ssize_t mode_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t count)
{
    struct my_device *mydev = dev_get_drvdata(dev);

    if (sysfs_streq(buf, "off"))
        mydev->mode = 0;
    else if (sysfs_streq(buf, "low"))
        mydev->mode = 1;
    else if (sysfs_streq(buf, "medium"))
        mydev->mode = 2;
    else if (sysfs_streq(buf, "high"))
        mydev->mode = 3;
    else
        return -EINVAL;

    return count;
}
static DEVICE_ATTR_RW(mode);

/* Enabled attribute (bool) */
static ssize_t enabled_show(struct device *dev,
                            struct device_attribute *attr, char *buf)
{
    struct my_device *mydev = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", mydev->enabled);
}

static ssize_t enabled_store(struct device *dev,
                             struct device_attribute *attr,
                             const char *buf, size_t count)
{
    struct my_device *mydev = dev_get_drvdata(dev);
    bool val;

    if (kstrtobool(buf, &val))
        return -EINVAL;

    mydev->enabled = val;
    return count;
}
static DEVICE_ATTR_RW(enabled);

static struct attribute *my_attrs[] = {
    &dev_attr_value.attr,
    &dev_attr_mode.attr,
    &dev_attr_enabled.attr,
    NULL,
};
ATTRIBUTE_GROUPS(my);

static int my_probe(struct platform_device *pdev)
{
    struct my_device *mydev;

    mydev = devm_kzalloc(&pdev->dev, sizeof(*mydev), GFP_KERNEL);
    if (!mydev)
        return -ENOMEM;

    mydev->value = 50;
    mydev->mode = 1;
    mydev->enabled = true;

    dev_set_drvdata(&pdev->dev, mydev);

    return 0;
}

static struct platform_driver my_driver = {
    .probe = my_probe,
    .driver = {
        .name = "my_device",
        .dev_groups = my_groups,
    },
};
module_platform_driver(my_driver);
```

Usage:

```bash
# Read values
cat /sys/devices/platform/my_device/value
cat /sys/devices/platform/my_device/mode
cat /sys/devices/platform/my_device/enabled

# Write values
echo 75 > /sys/devices/platform/my_device/value
echo high > /sys/devices/platform/my_device/mode
echo 0 > /sys/devices/platform/my_device/enabled
```

## Best Practices

1. **One value per file** - sysfs is "one value per file"
2. **Human-readable format** - text, not binary
3. **Validate input** - always check user input
4. **Return proper errors** - -EINVAL for bad input
5. **Use appropriate permissions** - don't expose sensitive data
6. **Document attributes** - in Documentation/ABI/

## Summary

- Device attributes create sysfs files
- Use DEVICE_ATTR_* macros for declaration
- Use attribute groups with `dev_groups` for automatic creation
- Binary attributes for large/binary data
- Conditional visibility with `is_visible`
- Always validate user input

## Next

Learn about [deferred probe]({% link part6/07-deferred-probe.md %}) for handling dependencies.
