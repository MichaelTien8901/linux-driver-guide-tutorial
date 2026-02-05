---
layout: default
title: "3.1 Character Device Concepts"
parent: "Part 3: Character Device Drivers"
nav_order: 1
---

# Character Device Concepts

Before writing a character device driver, you need to understand how the kernel identifies and manages devices.

## Major and Minor Numbers

Every device in Linux is identified by two numbers:

- **Major number**: Identifies the driver responsible for the device
- **Minor number**: Identifies the specific device instance within that driver

```bash
# View device numbers
ls -la /dev/ttyS*
# crw-rw---- 1 root dialout 4, 64 Jan 15 10:00 /dev/ttyS0
# crw-rw---- 1 root dialout 4, 65 Jan 15 10:00 /dev/ttyS1
#                           ^  ^^
#                           |  |+-- Minor number
#                           +------ Major number
```

### Device Number Type

The kernel uses `dev_t` to store device numbers:

```c
#include <linux/types.h>
#include <linux/kdev_t.h>

dev_t dev;

/* Create dev_t from major and minor */
dev = MKDEV(major, minor);

/* Extract major and minor from dev_t */
unsigned int major = MAJOR(dev);
unsigned int minor = MINOR(dev);
```

### Number Ranges

| Range | Bits | Maximum |
|-------|------|---------|
| Major | 12 bits | 0-4095 |
| Minor | 20 bits | 0-1048575 |

## Obtaining Device Numbers

You can either request specific numbers or let the kernel assign them dynamically.

### Dynamic Allocation (Recommended)

```c
#include <linux/fs.h>

static dev_t dev_num;

static int __init my_init(void)
{
    int ret;

    /* Request a range of minor numbers starting at 0 */
    ret = alloc_chrdev_region(&dev_num,     /* Output: first device number */
                              0,             /* First minor number requested */
                              1,             /* Number of devices */
                              "mydevice");   /* Name in /proc/devices */
    if (ret < 0) {
        pr_err("Failed to allocate chrdev region\n");
        return ret;
    }

    pr_info("Allocated major=%d, minor=%d\n",
            MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit my_exit(void)
{
    unregister_chrdev_region(dev_num, 1);
}
```

### Static Allocation

```c
#include <linux/fs.h>

#define MY_MAJOR 240  /* Use an unassigned number */
#define MY_MINOR 0

static dev_t dev_num;

static int __init my_init(void)
{
    int ret;

    dev_num = MKDEV(MY_MAJOR, MY_MINOR);

    ret = register_chrdev_region(dev_num, 1, "mydevice");
    if (ret < 0) {
        pr_err("Failed to register chrdev region\n");
        return ret;
    }

    return 0;
}
```

{: .warning }
Static allocation can cause conflicts. Always prefer dynamic allocation with `alloc_chrdev_region()`.

## Device Nodes

Device nodes are special files in `/dev/` that provide the user-space interface:

```mermaid
flowchart LR
    App["Application"] -->|"open('/dev/mydev')"| DevNode["/dev/mydev<br/>major:minor"]
    DevNode --> VFS["VFS"]
    VFS -->|"major lookup"| Driver["Your Driver"]

    style DevNode fill:#8f8a73,stroke:#f9a825
```

### Creating Device Nodes

There are two ways to create device nodes:

**1. Manual with mknod (Traditional)**

```bash
# Create character device node
sudo mknod /dev/mydevice c 240 0
#                        ^  ^  ^
#                        |  |  +-- Minor number
#                        |  +----- Major number
#                        +-------- 'c' for character device
```

**2. Automatic with udev (Recommended)**

```c
#include <linux/device.h>

static struct class *my_class;
static struct device *my_device;

static int __init my_init(void)
{
    /* ... allocate device number ... */

    /* Create device class (appears in /sys/class/) */
    my_class = class_create("my_class");
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        goto err_class;
    }

    /* Create device (triggers udev to create /dev node) */
    my_device = device_create(my_class,   /* Class */
                              NULL,        /* Parent device */
                              dev_num,     /* Device number */
                              NULL,        /* Device data */
                              "mydevice"); /* Device name */
    if (IS_ERR(my_device)) {
        ret = PTR_ERR(my_device);
        goto err_device;
    }

    return 0;

err_device:
    class_destroy(my_class);
err_class:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit my_exit(void)
{
    device_destroy(my_class, dev_num);
    class_destroy(my_class);```


    unregister_chrdev_region(dev_num, 1);
}
```

After loading the module, udev automatically creates `/dev/mydevice`.

## Viewing Device Information

### /proc/devices

Lists registered character and block drivers:

```bash
cat /proc/devices
# Character devices:
#   1 mem
#   4 tty
#   5 /dev/tty
# 240 mydevice    <-- Your driver
```

### /sys/class

Shows device classes and their devices:

```bash
ls /sys/class/my_class/
# mydevice

ls -la /sys/class/my_class/mydevice/
# dev      -> Contains "240:0"
# device   -> Link to device
# subsystem -> Link to class
# uevent   -> Triggers udev events
```

## Multiple Device Instances

A single driver can manage multiple devices:

```c
#define NUM_DEVICES 4
static dev_t dev_base;
static struct device *devices[NUM_DEVICES];

static int __init my_init(void)
{
    int i, ret;

    /* Allocate range of device numbers */
    ret = alloc_chrdev_region(&dev_base, 0, NUM_DEVICES, "mydevice");
    if (ret < 0)
        return ret;

    my_class = class_create("my_class");
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        goto err_class;
    }

    /* Create multiple device nodes */
    for (i = 0; i < NUM_DEVICES; i++) {
        devices[i] = device_create(my_class, NULL,
                                   MKDEV(MAJOR(dev_base), i),
                                   NULL, "mydevice%d", i);
        if (IS_ERR(devices[i])) {
            ret = PTR_ERR(devices[i]);
            goto err_devices;
        }
    }

    return 0;

err_devices:
    while (--i >= 0)
        device_destroy(my_class, MKDEV(MAJOR(dev_base), i));
    class_destroy(my_class);
err_class:
    unregister_chrdev_region(dev_base, NUM_DEVICES);
    return ret;
}
```

Result:

```bash
ls /dev/mydevice*
# /dev/mydevice0  /dev/mydevice1  /dev/mydevice2  /dev/mydevice3
```

## Character vs Block Devices

| Aspect | Character Device | Block Device |
|--------|------------------|--------------|
| Data transfer | Byte streams | Fixed-size blocks |
| Random access | Optional | Required |
| Buffering | Minimal | Block cache |
| Examples | Serial, keyboard | Disk, SSD |
| Dev type | `c` in ls -la | `b` in ls -la |

```bash
ls -la /dev/sda /dev/ttyS0
# brw-rw---- 1 root disk   8, 0 Jan 15 10:00 /dev/sda     # Block
# crw-rw---- 1 root dialout 4, 64 Jan 15 10:00 /dev/ttyS0 # Char
```

## Summary

- Devices are identified by major (driver) and minor (instance) numbers
- Use `alloc_chrdev_region()` for dynamic number allocation
- Use `class_create()` and `device_create()` for automatic device node creation
- Device nodes in `/dev/` are the user-space interface to your driver

## Next

Learn about [file operations]({% link part3/02-file-operations.md %}) - the functions that handle user requests.
