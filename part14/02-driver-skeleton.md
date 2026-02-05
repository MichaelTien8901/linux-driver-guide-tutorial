---
layout: default
title: "14.2 Driver Skeleton"
parent: "Part 14: USB Drivers"
nav_order: 2
---

# USB Driver Skeleton

This chapter shows the standard USB driver structure.

## Minimal USB Driver

```c
#include <linux/module.h>
#include <linux/usb.h>

struct my_usb {
    struct usb_device *udev;
    struct usb_interface *intf;
    unsigned char bulk_in_addr;
    unsigned char bulk_out_addr;
};

/* ============ Probe/Disconnect ============ */

static int my_probe(struct usb_interface *intf,
                    const struct usb_device_id *id)
{
    struct my_usb *dev;
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *ep;
    int i;

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->udev = interface_to_usbdev(intf);
    dev->intf = intf;

    /* Find bulk endpoints */
    iface_desc = intf->cur_altsetting;
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        ep = &iface_desc->endpoint[i].desc;

        if (usb_endpoint_is_bulk_in(ep))
            dev->bulk_in_addr = ep->bEndpointAddress;
        else if (usb_endpoint_is_bulk_out(ep))
            dev->bulk_out_addr = ep->bEndpointAddress;
    }

    usb_set_intfdata(intf, dev);

    dev_info(&intf->dev, "USB device attached\n");
    return 0;
}

static void my_disconnect(struct usb_interface *intf)
{
    struct my_usb *dev = usb_get_intfdata(intf);

    usb_set_intfdata(intf, NULL);
    kfree(dev);

    dev_info(&intf->dev, "USB device disconnected\n");
}

/* ============ Device ID Table ============ */

static const struct usb_device_id my_id_table[] = {
    { USB_DEVICE(0x1234, 0x5678) },  /* Your device VID:PID */
    { }
};
MODULE_DEVICE_TABLE(usb, my_id_table);

/* ============ USB Driver Structure ============ */

static struct usb_driver my_driver = {
    .name = "my_usb_driver",
    .id_table = my_id_table,
    .probe = my_probe,
    .disconnect = my_disconnect,
};

module_usb_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("My USB Driver");
```

## Key Components

### Device ID Table

Match devices by VID:PID or class:

```c
static const struct usb_device_id my_id_table[] = {
    /* By VID:PID */
    { USB_DEVICE(0x1234, 0x5678) },

    /* By class (e.g., all HID devices) */
    { USB_INTERFACE_INFO(USB_CLASS_HID, 0, 0) },

    /* By VID with any PID */
    { USB_DEVICE(0x1234, USB_DEVICE_ID_MATCH_INT_INFO) },

    { }  /* Terminator */
};
```

### Probe Function

Called when your device is detected:

```c
static int my_probe(struct usb_interface *intf,
                    const struct usb_device_id *id)
{
    /* 1. Allocate private data */
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);

    /* 2. Get USB device */
    dev->udev = interface_to_usbdev(intf);

    /* 3. Find endpoints */
    /* ... iterate and find bulk/int endpoints ... */

    /* 4. Store private data */
    usb_set_intfdata(intf, dev);

    /* 5. Create user interface (optional) */
    /* e.g., misc device, sysfs attributes */

    return 0;
}
```

### Disconnect Function

Called when device is unplugged:

```c
static void my_disconnect(struct usb_interface *intf)
{
    struct my_usb *dev = usb_get_intfdata(intf);

    /* 1. Remove user interface */

    /* 2. Cancel any pending URBs */
    usb_kill_urb(dev->urb);

    /* 3. Clear interface data */
    usb_set_intfdata(intf, NULL);

    /* 4. Free resources */
    usb_free_urb(dev->urb);
    kfree(dev);
}
```

{: .warning }
> Disconnect can be called while your driver is in use. Cancel all URBs and ensure no ongoing operations before freeing memory.

## USB Driver Registration

```c
/* Modern way - handles module_init/exit */
module_usb_driver(my_driver);

/* Equivalent to: */
static int __init my_init(void)
{
    return usb_register(&my_driver);
}
static void __exit my_exit(void)
{
    usb_deregister(&my_driver);
}
module_init(my_init);
module_exit(my_exit);
```

## Finding Your Device

Check your device's VID:PID:

```bash
# List USB devices
lsusb
# Example output: Bus 001 Device 005: ID 1234:5678 My Device

# Detailed info
lsusb -v -d 1234:5678
```

## Summary

| Component | Purpose |
|-----------|---------|
| `usb_device_id` | Match criteria for your device |
| `probe()` | Initialize when device attached |
| `disconnect()` | Cleanup when device removed |
| `usb_driver` | Driver registration structure |
| `module_usb_driver()` | Registration macro |

## Further Reading

- [Writing USB Drivers](https://docs.kernel.org/driver-api/usb/writing_usb_driver.html) - Official tutorial
- [USB Core API](https://docs.kernel.org/driver-api/usb/usb.html) - Function reference
