---
layout: default
title: "14.3 USB Transfers"
parent: "Part 14: USB Drivers"
nav_order: 3
---

# USB Transfers

This chapter covers bulk and control transfers - the most common types.

## Synchronous Bulk Transfer

For simple, occasional transfers, use the blocking API:

```c
/* Write to device */
int ret = usb_bulk_msg(
    dev->udev,
    usb_sndbulkpipe(dev->udev, dev->bulk_out_addr),
    buffer,       /* Data to send */
    len,          /* Length */
    &actual_len,  /* Actual bytes sent */
    5000          /* Timeout in ms */
);

/* Read from device */
ret = usb_bulk_msg(
    dev->udev,
    usb_rcvbulkpipe(dev->udev, dev->bulk_in_addr),
    buffer,
    buffer_size,
    &actual_len,
    5000
);

if (ret < 0)
    dev_err(&dev->intf->dev, "Bulk transfer failed: %d\n", ret);
```

{: .note }
> `usb_bulk_msg()` sleeps. Don't use it in interrupt context or with locks held.

## Asynchronous Bulk Transfer

For continuous or high-performance I/O:

```c
struct my_usb {
    struct urb *urb;
    unsigned char *buffer;
    /* ... */
};

/* Callback when transfer completes */
static void my_bulk_callback(struct urb *urb)
{
    struct my_usb *dev = urb->context;

    switch (urb->status) {
    case 0:
        /* Success */
        dev_dbg(&dev->intf->dev, "Received %d bytes\n",
                urb->actual_length);
        /* Process data in dev->buffer */

        /* Resubmit for continuous reading */
        usb_submit_urb(urb, GFP_ATOMIC);
        break;

    case -ENOENT:
    case -ECONNRESET:
    case -ESHUTDOWN:
        /* URB was killed - don't resubmit */
        break;

    default:
        dev_err(&dev->intf->dev, "URB error: %d\n", urb->status);
        break;
    }
}

/* In probe: setup the URB */
static int setup_bulk_urb(struct my_usb *dev)
{
    dev->urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!dev->urb)
        return -ENOMEM;

    dev->buffer = kmalloc(64, GFP_KERNEL);
    if (!dev->buffer) {
        usb_free_urb(dev->urb);
        return -ENOMEM;
    }

    usb_fill_bulk_urb(
        dev->urb,
        dev->udev,
        usb_rcvbulkpipe(dev->udev, dev->bulk_in_addr),
        dev->buffer,
        64,               /* Buffer size */
        my_bulk_callback,
        dev               /* Context for callback */
    );

    return 0;
}

/* Start reading */
static int start_reading(struct my_usb *dev)
{
    return usb_submit_urb(dev->urb, GFP_KERNEL);
}

/* Stop reading */
static void stop_reading(struct my_usb *dev)
{
    usb_kill_urb(dev->urb);
}
```

## Control Transfers

For device configuration and vendor commands:

```c
/* Standard control request */
int ret = usb_control_msg(
    dev->udev,
    usb_sndctrlpipe(dev->udev, 0),  /* EP0 */
    request,       /* bRequest */
    requesttype,   /* bmRequestType */
    value,         /* wValue */
    index,         /* wIndex */
    data,          /* Data buffer (can be NULL) */
    size,          /* wLength */
    5000           /* Timeout */
);

/* Example: Vendor-specific command */
ret = usb_control_msg(
    dev->udev,
    usb_sndctrlpipe(dev->udev, 0),
    0x01,          /* Vendor request 1 */
    USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
    0x0000,        /* Value */
    0x0000,        /* Index */
    NULL,          /* No data */
    0,
    5000
);
```

Request type flags:
- `USB_TYPE_STANDARD`, `USB_TYPE_CLASS`, `USB_TYPE_VENDOR`
- `USB_RECIP_DEVICE`, `USB_RECIP_INTERFACE`, `USB_RECIP_ENDPOINT`
- `USB_DIR_OUT`, `USB_DIR_IN`

## Interrupt Transfers

For small periodic data (HID events, status):

```c
/* Similar to bulk, but use interrupt functions */
usb_fill_int_urb(
    urb,
    dev->udev,
    usb_rcvintpipe(dev->udev, dev->int_in_addr),
    dev->buffer,
    buffer_size,
    my_int_callback,
    dev,
    endpoint->bInterval  /* Polling interval */
);

usb_submit_urb(urb, GFP_KERNEL);
```

## URB Status Codes

| Status | Meaning |
|--------|---------|
| `0` | Success |
| `-ENOENT` | URB was killed |
| `-ECONNRESET` | URB was unlinked |
| `-ESHUTDOWN` | Device disconnected |
| `-EPIPE` | Endpoint stalled |
| `-EOVERFLOW` | Data overrun |
| `-ETIMEDOUT` | Timeout |

## Handling Disconnect

Always handle the case where the device is unplugged during transfer:

```c
static void my_disconnect(struct usb_interface *intf)
{
    struct my_usb *dev = usb_get_intfdata(intf);

    /* Kill any pending URBs - blocks until callback returns */
    usb_kill_urb(dev->urb);

    /* Now safe to free */
    usb_free_urb(dev->urb);
    kfree(dev->buffer);
    kfree(dev);
}
```

## Summary

| Function | Use Case |
|----------|----------|
| `usb_bulk_msg()` | Simple blocking bulk transfer |
| `usb_control_msg()` | Blocking control transfer |
| `usb_fill_bulk_urb()` | Prepare async bulk URB |
| `usb_fill_int_urb()` | Prepare async interrupt URB |
| `usb_submit_urb()` | Submit URB (non-blocking) |
| `usb_kill_urb()` | Cancel URB (blocks until done) |

## Further Reading

- [URB Documentation](https://docs.kernel.org/driver-api/usb/URB.html) - Complete URB reference
- [USB Core API](https://docs.kernel.org/driver-api/usb/usb.html) - All functions
