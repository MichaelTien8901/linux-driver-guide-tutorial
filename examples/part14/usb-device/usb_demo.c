// SPDX-License-Identifier: GPL-2.0
/*
 * USB Device Demo Driver
 *
 * Demonstrates USB driver fundamentals:
 * - Device matching by VID:PID
 * - Endpoint discovery in probe
 * - Bulk transfers (sync and async)
 * - Proper disconnect handling
 *
 * This driver creates a misc device for userspace access.
 * Write data to send to device, read to receive.
 *
 * NOTE: Modify VID:PID to match your actual device, or use
 * the loopback test with dummy_hcd for testing.
 */

#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>

#define DRIVER_NAME "usb_demo"
#define BUFFER_SIZE 64

/* Change these to match your device */
#define DEMO_VID 0x1234
#define DEMO_PID 0x5678

struct usb_demo {
    struct usb_device *udev;
    struct usb_interface *intf;
    struct miscdevice misc;

    /* Endpoints */
    unsigned char bulk_in_addr;
    unsigned char bulk_out_addr;
    size_t bulk_in_size;

    /* Buffers */
    unsigned char *bulk_in_buffer;
    unsigned char *bulk_out_buffer;

    /* Synchronization */
    struct mutex io_mutex;
    bool connected;

    /* For async read */
    struct urb *read_urb;
    struct completion read_done;
    int read_status;
    int read_actual;
};

/* ============ File Operations ============ */

static int usb_demo_open(struct inode *inode, struct file *file)
{
    struct usb_demo *dev;

    dev = container_of(file->private_data, struct usb_demo, misc);
    file->private_data = dev;

    if (!dev->connected)
        return -ENODEV;

    return 0;
}

static ssize_t usb_demo_read(struct file *file, char __user *buf,
                              size_t count, loff_t *ppos)
{
    struct usb_demo *dev = file->private_data;
    int ret;
    int actual_len;

    if (!dev->connected)
        return -ENODEV;

    if (count > dev->bulk_in_size)
        count = dev->bulk_in_size;

    mutex_lock(&dev->io_mutex);

    if (!dev->connected) {
        ret = -ENODEV;
        goto unlock;
    }

    /* Synchronous bulk read */
    ret = usb_bulk_msg(
        dev->udev,
        usb_rcvbulkpipe(dev->udev, dev->bulk_in_addr),
        dev->bulk_in_buffer,
        count,
        &actual_len,
        5000  /* 5 second timeout */
    );

    if (ret) {
        dev_err(&dev->intf->dev, "Bulk read failed: %d\n", ret);
        goto unlock;
    }

    if (copy_to_user(buf, dev->bulk_in_buffer, actual_len)) {
        ret = -EFAULT;
        goto unlock;
    }

    ret = actual_len;

unlock:
    mutex_unlock(&dev->io_mutex);
    return ret;
}

static ssize_t usb_demo_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *ppos)
{
    struct usb_demo *dev = file->private_data;
    int ret;
    int actual_len;

    if (!dev->connected)
        return -ENODEV;

    if (count > BUFFER_SIZE)
        count = BUFFER_SIZE;

    mutex_lock(&dev->io_mutex);

    if (!dev->connected) {
        ret = -ENODEV;
        goto unlock;
    }

    if (copy_from_user(dev->bulk_out_buffer, buf, count)) {
        ret = -EFAULT;
        goto unlock;
    }

    /* Synchronous bulk write */
    ret = usb_bulk_msg(
        dev->udev,
        usb_sndbulkpipe(dev->udev, dev->bulk_out_addr),
        dev->bulk_out_buffer,
        count,
        &actual_len,
        5000
    );

    if (ret) {
        dev_err(&dev->intf->dev, "Bulk write failed: %d\n", ret);
        goto unlock;
    }

    ret = actual_len;

unlock:
    mutex_unlock(&dev->io_mutex);
    return ret;
}

static const struct file_operations usb_demo_fops = {
    .owner = THIS_MODULE,
    .open = usb_demo_open,
    .read = usb_demo_read,
    .write = usb_demo_write,
};

/* ============ USB Probe/Disconnect ============ */

static int usb_demo_probe(struct usb_interface *intf,
                          const struct usb_device_id *id)
{
    struct usb_demo *dev;
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *ep;
    int i, ret;

    /* Allocate device structure */
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->udev = interface_to_usbdev(intf);
    dev->intf = intf;
    mutex_init(&dev->io_mutex);
    init_completion(&dev->read_done);
    dev->connected = true;

    /* Find bulk endpoints */
    iface_desc = intf->cur_altsetting;
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        ep = &iface_desc->endpoint[i].desc;

        if (usb_endpoint_is_bulk_in(ep) && !dev->bulk_in_addr) {
            dev->bulk_in_addr = ep->bEndpointAddress;
            dev->bulk_in_size = usb_endpoint_maxp(ep);
        }
        if (usb_endpoint_is_bulk_out(ep) && !dev->bulk_out_addr) {
            dev->bulk_out_addr = ep->bEndpointAddress;
        }
    }

    if (!dev->bulk_in_addr || !dev->bulk_out_addr) {
        dev_err(&intf->dev, "Could not find bulk endpoints\n");
        ret = -ENODEV;
        goto error;
    }

    /* Allocate buffers */
    dev->bulk_in_buffer = kmalloc(dev->bulk_in_size, GFP_KERNEL);
    dev->bulk_out_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!dev->bulk_in_buffer || !dev->bulk_out_buffer) {
        ret = -ENOMEM;
        goto error;
    }

    /* Register misc device */
    dev->misc.minor = MISC_DYNAMIC_MINOR;
    dev->misc.name = DRIVER_NAME;
    dev->misc.fops = &usb_demo_fops;

    ret = misc_register(&dev->misc);
    if (ret) {
        dev_err(&intf->dev, "Failed to register misc device\n");
        goto error;
    }

    usb_set_intfdata(intf, dev);

    dev_info(&intf->dev, "USB demo device attached (IN:0x%02x OUT:0x%02x)\n",
             dev->bulk_in_addr, dev->bulk_out_addr);

    return 0;

error:
    kfree(dev->bulk_in_buffer);
    kfree(dev->bulk_out_buffer);
    kfree(dev);
    return ret;
}

static void usb_demo_disconnect(struct usb_interface *intf)
{
    struct usb_demo *dev = usb_get_intfdata(intf);

    /* Mark as disconnected */
    mutex_lock(&dev->io_mutex);
    dev->connected = false;
    mutex_unlock(&dev->io_mutex);

    /* Unregister misc device */
    misc_deregister(&dev->misc);

    usb_set_intfdata(intf, NULL);

    /* Free resources */
    kfree(dev->bulk_in_buffer);
    kfree(dev->bulk_out_buffer);
    kfree(dev);

    dev_info(&intf->dev, "USB demo device disconnected\n");
}

/* ============ Module Registration ============ */

static const struct usb_device_id usb_demo_id_table[] = {
    { USB_DEVICE(DEMO_VID, DEMO_PID) },
    { }
};
MODULE_DEVICE_TABLE(usb, usb_demo_id_table);

static struct usb_driver usb_demo_driver = {
    .name = DRIVER_NAME,
    .id_table = usb_demo_id_table,
    .probe = usb_demo_probe,
    .disconnect = usb_demo_disconnect,
};

module_usb_driver(usb_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("USB Device Demo Driver");
