---
layout: default
title: "14.6 User Space Interface"
parent: "Part 14: USB Drivers"
nav_order: 6
---

# User Space Interface

This chapter covers exposing USB devices to user space applications.

## Character Device Interface

The USB subsystem provides helpers to create `/dev` entries:

```c
#include <linux/usb.h>

static const struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
    .unlocked_ioctl = my_ioctl,
};

static struct usb_class_driver my_class = {
    .name       = "myusb%d",      /* /dev/myusb0, myusb1, ... */
    .fops       = &my_fops,
    .minor_base = 192,            /* Starting minor number */
};

static int my_probe(struct usb_interface *intf,
                    const struct usb_device_id *id)
{
    struct my_usb *dev;
    int ret;

    /* ... allocate and initialize dev ... */

    /* Register character device */
    ret = usb_register_dev(intf, &my_class);
    if (ret) {
        dev_err(&intf->dev, "Failed to register dev: %d\n", ret);
        return ret;
    }

    dev_info(&intf->dev, "USB device attached at /dev/myusb%d\n",
             intf->minor);

    return 0;
}

static void my_disconnect(struct usb_interface *intf)
{
    /* Unregister character device */
    usb_deregister_dev(intf, &my_class);

    /* ... rest of cleanup ... */
}
```

## File Operations Implementation

### Open and Release

Track open count to prevent disconnect during use:

```c
static int my_open(struct inode *inode, struct file *file)
{
    struct usb_interface *intf;
    struct my_usb *dev;

    /* Get interface from minor number */
    intf = usb_find_interface(&my_driver, iminor(inode));
    if (!intf)
        return -ENODEV;

    dev = usb_get_intfdata(intf);
    if (!dev)
        return -ENODEV;

    /* Prevent autosuspend while open */
    usb_autopm_get_interface(intf);

    /* Increment usage count */
    kref_get(&dev->kref);

    /* Store for read/write/ioctl */
    file->private_data = dev;

    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    struct my_usb *dev = file->private_data;

    if (!dev)
        return -ENODEV;

    /* Allow autosuspend */
    usb_autopm_put_interface(dev->intf);

    /* Decrement usage count */
    kref_put(&dev->kref, my_delete);

    return 0;
}
```

### Read

Read data from device:

```c
static ssize_t my_read(struct file *file, char __user *buf,
                       size_t count, loff_t *ppos)
{
    struct my_usb *dev = file->private_data;
    int ret, actual;

    if (!dev->connected)
        return -ENODEV;

    /* Perform USB read */
    ret = usb_bulk_msg(dev->udev,
                       usb_rcvbulkpipe(dev->udev, dev->bulk_in_addr),
                       dev->buffer,
                       min(count, (size_t)64),
                       &actual,
                       5000);

    if (ret < 0) {
        dev_err(&dev->intf->dev, "Read failed: %d\n", ret);
        return ret;
    }

    /* Copy to user space */
    if (copy_to_user(buf, dev->buffer, actual))
        return -EFAULT;

    return actual;
}
```

### Write

Write data to device:

```c
static ssize_t my_write(struct file *file, const char __user *buf,
                        size_t count, loff_t *ppos)
{
    struct my_usb *dev = file->private_data;
    int ret, actual;

    if (!dev->connected)
        return -ENODEV;

    if (count > 64)
        count = 64;

    /* Copy from user space */
    if (copy_from_user(dev->buffer, buf, count))
        return -EFAULT;

    /* Perform USB write */
    ret = usb_bulk_msg(dev->udev,
                       usb_sndbulkpipe(dev->udev, dev->bulk_out_addr),
                       dev->buffer,
                       count,
                       &actual,
                       5000);

    if (ret < 0) {
        dev_err(&dev->intf->dev, "Write failed: %d\n", ret);
        return ret;
    }

    return actual;
}
```

### IOCTL

Custom commands:

```c
#define MY_USB_IOCTL_RESET   _IO('U', 1)
#define MY_USB_IOCTL_GET_FW  _IOR('U', 2, u32)

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct my_usb *dev = file->private_data;
    void __user *argp = (void __user *)arg;
    int ret = 0;

    if (!dev->connected)
        return -ENODEV;

    switch (cmd) {
    case MY_USB_IOCTL_RESET:
        ret = usb_reset_device(dev->udev);
        break;

    case MY_USB_IOCTL_GET_FW:
    {
        u32 fw_version;
        /* Read firmware version via control transfer */
        ret = usb_control_msg(dev->udev,
                              usb_rcvctrlpipe(dev->udev, 0),
                              0x01,  /* Vendor request: get FW */
                              USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
                              0, 0,
                              &fw_version, sizeof(fw_version),
                              5000);
        if (ret == sizeof(fw_version)) {
            if (copy_to_user(argp, &fw_version, sizeof(fw_version)))
                return -EFAULT;
            ret = 0;
        }
        break;
    }

    default:
        ret = -ENOTTY;
    }

    return ret;
}
```

## Poll Support

Allow `select()`/`poll()` on the device:

```c
static __poll_t my_poll(struct file *file, poll_table *wait)
{
    struct my_usb *dev = file->private_data;
    __poll_t mask = 0;

    poll_wait(file, &dev->read_wait, wait);
    poll_wait(file, &dev->write_wait, wait);

    if (!dev->connected)
        return EPOLLERR | EPOLLHUP;

    if (dev->data_available)
        mask |= EPOLLIN | EPOLLRDNORM;

    if (dev->can_write)
        mask |= EPOLLOUT | EPOLLWRNORM;

    return mask;
}

static const struct file_operations my_fops = {
    /* ... */
    .poll = my_poll,
};
```

Wake up waiters in your URB callback:

```c
static void my_read_callback(struct urb *urb)
{
    struct my_usb *dev = urb->context;

    if (urb->status == 0) {
        dev->data_available = true;
        wake_up_interruptible(&dev->read_wait);
    }
}
```

## Reference Counting

Properly handle device lifetime:

```c
struct my_usb {
    struct usb_device *udev;
    struct usb_interface *intf;
    struct kref kref;
    bool connected;
    /* ... */
};

static void my_delete(struct kref *kref)
{
    struct my_usb *dev = container_of(kref, struct my_usb, kref);

    usb_put_dev(dev->udev);
    kfree(dev->buffer);
    kfree(dev);
}

static int my_probe(struct usb_interface *intf,
                    const struct usb_device_id *id)
{
    struct my_usb *dev;

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    kref_init(&dev->kref);
    dev->udev = usb_get_dev(interface_to_usbdev(intf));
    dev->intf = intf;
    dev->connected = true;

    /* ... rest of probe ... */
    return 0;
}

static void my_disconnect(struct usb_interface *intf)
{
    struct my_usb *dev = usb_get_intfdata(intf);

    usb_deregister_dev(intf, &my_class);

    /* Mark disconnected - ongoing I/O will fail */
    dev->connected = false;

    /* Wake any sleeping readers/writers */
    wake_up_interruptible(&dev->read_wait);
    wake_up_interruptible(&dev->write_wait);

    usb_set_intfdata(intf, NULL);

    /* Release our reference */
    kref_put(&dev->kref, my_delete);
}
```

## User Space Example

```c
/* User space code */
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MY_USB_IOCTL_RESET   _IO('U', 1)
#define MY_USB_IOCTL_GET_FW  _IOR('U', 2, unsigned int)

int main()
{
    int fd = open("/dev/myusb0", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Read firmware version */
    unsigned int fw_version;
    if (ioctl(fd, MY_USB_IOCTL_GET_FW, &fw_version) == 0) {
        printf("Firmware: %u\n", fw_version);
    }

    /* Write data */
    char data[] = "Hello USB";
    write(fd, data, sizeof(data));

    /* Read response */
    char buf[64];
    int n = read(fd, buf, sizeof(buf));
    printf("Received %d bytes\n", n);

    close(fd);
    return 0;
}
```

## Summary

| Function | Purpose |
|----------|---------|
| `usb_register_dev()` | Create /dev entry |
| `usb_deregister_dev()` | Remove /dev entry |
| `usb_find_interface()` | Get interface from minor |
| `kref_get()`/`kref_put()` | Reference counting |
| `usb_get_dev()`/`usb_put_dev()` | USB device refcount |

**Key patterns:**
1. Use `usb_class_driver` for automatic /dev creation
2. Track `connected` flag for disconnect handling
3. Use kref for proper lifetime management
4. Wake waiters on disconnect

## Further Reading

- [USB Skeleton Driver](https://elixir.bootlin.com/linux/v6.6/source/drivers/usb/usb-skeleton.c) - Reference implementation
- [USB Core API](https://docs.kernel.org/driver-api/usb/usb.html) - Registration functions
