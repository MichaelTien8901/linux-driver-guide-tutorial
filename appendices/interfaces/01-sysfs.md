---
layout: default
title: "B.1 sysfs"
parent: "Appendix B: Kernel Interfaces"
nav_order: 1
---

# sysfs Attributes

Expose device properties through `/sys/`.

## DEVICE_ATTR

Create read/write attributes for your device:

```c
/* Read-only attribute */
static ssize_t status_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    struct my_dev *mydev = dev_get_drvdata(dev);
    return sysfs_emit(buf, "%d\n", mydev->status);
}
static DEVICE_ATTR_RO(status);

/* Read-write attribute */
static ssize_t config_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    struct my_dev *mydev = dev_get_drvdata(dev);
    return sysfs_emit(buf, "%u\n", mydev->config);
}

static ssize_t config_store(struct device *dev,
                            struct device_attribute *attr,
                            const char *buf, size_t count)
{
    struct my_dev *mydev = dev_get_drvdata(dev);
    unsigned int val;

    if (kstrtouint(buf, 0, &val))
        return -EINVAL;

    mydev->config = val;
    return count;
}
static DEVICE_ATTR_RW(config);
```

## Attribute Groups

Group attributes together (preferred method):

```c
static struct attribute *my_attrs[] = {
    &dev_attr_status.attr,
    &dev_attr_config.attr,
    NULL,
};
ATTRIBUTE_GROUPS(my);

/* In driver */
static struct platform_driver my_driver = {
    .driver = {
        .name = "my_driver",
        .dev_groups = my_groups,  /* Auto-created on probe */
    },
};
```

{: .tip }
> Use `dev_groups` in driver struct. Attributes are automatically created during probe and removed during remove.

## Using sysfs_emit

Always use `sysfs_emit()` instead of `sprintf()`:

```c
/* Good */
return sysfs_emit(buf, "%d\n", value);

/* Bad - may overflow */
return sprintf(buf, "%d\n", value);
```

## Binary Attributes

For binary data (firmware dumps, etc.):

```c
static ssize_t data_read(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr,
                         char *buf, loff_t off, size_t count)
{
    struct device *dev = kobj_to_dev(kobj);
    struct my_dev *mydev = dev_get_drvdata(dev);

    return memory_read_from_buffer(buf, count, &off,
                                   mydev->data, mydev->data_size);
}
static BIN_ATTR_RO(data, DATA_SIZE);
```

## Summary

| Macro | Creates |
|-------|---------|
| `DEVICE_ATTR_RO(name)` | Read-only attribute |
| `DEVICE_ATTR_WO(name)` | Write-only attribute |
| `DEVICE_ATTR_RW(name)` | Read-write attribute |
| `ATTRIBUTE_GROUPS(name)` | Attribute group |

## Further Reading

- [sysfs Documentation](https://docs.kernel.org/filesystems/sysfs.html)
- [Device Attributes](https://docs.kernel.org/driver-api/driver-model/device.html)
