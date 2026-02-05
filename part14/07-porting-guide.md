---
layout: default
title: "14.7 Porting from Custom I/O"
parent: "Part 14: USB Drivers"
nav_order: 7
---

# Porting USB Drivers from Custom I/O

This chapter covers migrating USB device drivers from proprietary or custom I/O implementations to the standard Linux USB subsystem.

## Common Migration Scenarios

| From | To | Key Changes |
|------|-----|-------------|
| User-space libusb | Kernel USB driver | Move to URBs, handle interrupts |
| Vendor SDK/library | Standard usb_driver | Replace vendor APIs with kernel APIs |
| Direct I/O port access | USB subsystem | Use USB transfer functions |
| Polling-based | Interrupt/callback | Async URB model |
| Windows driver port | Linux USB | Different API, similar concepts |

## Mapping Concepts

### libusb to Kernel Driver

```
libusb Concept          →  Kernel Equivalent
─────────────────────────────────────────────
libusb_device_handle    →  struct usb_device *
libusb_open()           →  probe() callback
libusb_close()          →  disconnect() callback
libusb_bulk_transfer()  →  usb_bulk_msg() or URB
libusb_control_transfer →  usb_control_msg()
libusb_interrupt_transfer → usb_fill_int_urb() + submit
libusb_get_device_descriptor → Already in usb_device
```

### Example: libusb to Kernel

**Before (libusb user-space):**
```c
/* User-space libusb code */
libusb_device_handle *handle;
libusb_open(dev, &handle);

/* Bulk write */
int transferred;
libusb_bulk_transfer(handle, 0x02, data, len, &transferred, 5000);

/* Bulk read */
libusb_bulk_transfer(handle, 0x81, buffer, sizeof(buffer), &transferred, 5000);

libusb_close(handle);
```

**After (kernel driver):**
```c
/* Kernel USB driver */
static int my_probe(struct usb_interface *intf,
                    const struct usb_device_id *id)
{
    struct my_dev *dev;
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    dev->udev = interface_to_usbdev(intf);
    dev->bulk_out = 0x02;
    dev->bulk_in = 0x81;
    usb_set_intfdata(intf, dev);
    return 0;
}

static int my_write(struct my_dev *dev, u8 *data, int len)
{
    int actual;
    return usb_bulk_msg(dev->udev,
                        usb_sndbulkpipe(dev->udev, dev->bulk_out),
                        data, len, &actual, 5000);
}

static int my_read(struct my_dev *dev, u8 *buffer, int size)
{
    int actual;
    int ret = usb_bulk_msg(dev->udev,
                           usb_rcvbulkpipe(dev->udev, dev->bulk_in),
                           buffer, size, &actual, 5000);
    return ret < 0 ? ret : actual;
}

static void my_disconnect(struct usb_interface *intf)
{
    struct my_dev *dev = usb_get_intfdata(intf);
    kfree(dev);
}
```

## Porting Vendor SDK Code

### Step 1: Identify Transfer Types

Map vendor functions to USB transfer types:

```c
/* Vendor SDK (example) */
vendor_send_command(handle, cmd, param);     /* → Control transfer */
vendor_write_data(handle, buf, len);         /* → Bulk OUT */
vendor_read_data(handle, buf, len);          /* → Bulk IN */
vendor_get_status(handle, &status);          /* → Interrupt IN or Control */
```

### Step 2: Replace with Kernel APIs

```c
/* Control transfer replacement */
static int send_command(struct my_dev *dev, u8 cmd, u16 param)
{
    return usb_control_msg(dev->udev,
                           usb_sndctrlpipe(dev->udev, 0),
                           cmd,                              /* bRequest */
                           USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                           param,                            /* wValue */
                           0,                                /* wIndex */
                           NULL, 0,                          /* No data stage */
                           5000);
}

/* Bulk transfer replacement */
static int write_data(struct my_dev *dev, void *buf, int len)
{
    int actual;
    return usb_bulk_msg(dev->udev,
                        usb_sndbulkpipe(dev->udev, dev->bulk_out_ep),
                        buf, len, &actual, 5000);
}
```

### Step 3: Handle Async Operations

Convert polling to callback-based:

**Before (polling):**
```c
/* Vendor SDK polling */
while (!vendor_data_ready(handle)) {
    msleep(10);
}
vendor_read_data(handle, buffer, size);
```

**After (interrupt URB):**
```c
/* Kernel async with URB */
static void status_callback(struct urb *urb)
{
    struct my_dev *dev = urb->context;

    if (urb->status == 0) {
        /* Data ready - process it */
        process_data(dev, dev->int_buffer, urb->actual_length);

        /* Resubmit for next interrupt */
        usb_submit_urb(urb, GFP_ATOMIC);
    }
}

static int start_status_polling(struct my_dev *dev)
{
    usb_fill_int_urb(dev->int_urb, dev->udev,
                     usb_rcvintpipe(dev->udev, dev->int_ep),
                     dev->int_buffer, 8,
                     status_callback, dev,
                     dev->int_interval);
    return usb_submit_urb(dev->int_urb, GFP_KERNEL);
}
```

## Porting from Windows Drivers

### WDM/KMDF to Linux Mapping

```
Windows Concept              →  Linux Equivalent
───────────────────────────────────────────────────
DRIVER_OBJECT               →  struct usb_driver
DEVICE_OBJECT               →  struct usb_interface
IRP                         →  struct urb
IoCreateDevice              →  probe() callback
IoDeleteDevice              →  disconnect() callback
UsbBuildInterruptOrBulkTransferRequest → usb_fill_bulk_urb
WdfUsbTargetPipeReadSynchronously → usb_bulk_msg
WdfRequestSend              →  usb_submit_urb
```

### Example Conversion

**Windows KMDF:**
```c
// Windows driver
NTSTATUS MyDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    WDF_USB_DEVICE_CREATE_CONFIG config;
    WdfUsbTargetDeviceCreate(device, &config, &usbDevice);
    // ...
}

NTSTATUS MyRead(WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
    WdfUsbTargetPipeReadSynchronously(pipe, Request, NULL, &buffer, NULL);
}
```

**Linux equivalent:**
```c
/* Linux driver */
static int my_probe(struct usb_interface *intf,
                    const struct usb_device_id *id)
{
    struct my_dev *dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    dev->udev = interface_to_usbdev(intf);
    /* Find endpoints... */
    usb_set_intfdata(intf, dev);
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf,
                       size_t len, loff_t *ppos)
{
    struct my_dev *dev = file->private_data;
    int actual;
    int ret = usb_bulk_msg(dev->udev,
                           usb_rcvbulkpipe(dev->udev, dev->bulk_in),
                           dev->buffer, len, &actual, 5000);
    if (ret == 0 && copy_to_user(buf, dev->buffer, actual))
        return -EFAULT;
    return ret < 0 ? ret : actual;
}
```

## Protocol Layer Porting

### Preserving Device Protocol

Keep your protocol layer, replace I/O layer:

```c
/* Protocol layer (keep this) */
struct my_protocol {
    int (*send_cmd)(void *ctx, u8 cmd, u16 param);
    int (*read_data)(void *ctx, void *buf, int len);
    int (*write_data)(void *ctx, void *buf, int len);
};

/* Old I/O implementation (replace) */
static int old_send_cmd(void *ctx, u8 cmd, u16 param)
{
    return vendor_sdk_command(ctx, cmd, param);
}

/* New kernel I/O implementation */
static int kernel_send_cmd(void *ctx, u8 cmd, u16 param)
{
    struct my_dev *dev = ctx;
    return usb_control_msg(dev->udev,
                           usb_sndctrlpipe(dev->udev, 0),
                           cmd,
                           USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
                           param, 0, NULL, 0, 5000);
}

/* Wire up new implementation */
static const struct my_protocol kernel_ops = {
    .send_cmd = kernel_send_cmd,
    .read_data = kernel_read_data,
    .write_data = kernel_write_data,
};
```

## Buffer Management Changes

### User-Space to Kernel Buffers

```c
/* User-space: stack or heap buffer */
char buffer[64];
libusb_bulk_transfer(handle, ep, buffer, 64, &actual, 1000);

/* Kernel: must use kmalloc for DMA */
dev->buffer = kmalloc(64, GFP_KERNEL);
usb_bulk_msg(dev->udev, pipe, dev->buffer, 64, &actual, 1000);

/* Or use USB-specific allocation for DMA coherency */
dev->buffer = usb_alloc_coherent(dev->udev, 64, GFP_KERNEL, &dev->buffer_dma);
```

### DMA Considerations

```c
/* For high-performance transfers, use coherent buffers */
struct my_dev {
    void *buffer;
    dma_addr_t buffer_dma;
    struct urb *urb;
};

static int setup_dma_transfer(struct my_dev *dev)
{
    /* Allocate DMA-capable buffer */
    dev->buffer = usb_alloc_coherent(dev->udev, BUFFER_SIZE,
                                      GFP_KERNEL, &dev->buffer_dma);
    if (!dev->buffer)
        return -ENOMEM;

    /* Set up URB with DMA */
    dev->urb = usb_alloc_urb(0, GFP_KERNEL);
    usb_fill_bulk_urb(dev->urb, dev->udev, pipe,
                      dev->buffer, BUFFER_SIZE,
                      callback, dev);
    dev->urb->transfer_dma = dev->buffer_dma;
    dev->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    return 0;
}

static void cleanup_dma(struct my_dev *dev)
{
    usb_free_urb(dev->urb);
    usb_free_coherent(dev->udev, BUFFER_SIZE,
                      dev->buffer, dev->buffer_dma);
}
```

## Error Handling Migration

```c
/* Map vendor/libusb errors to kernel errors */
static int translate_error(int vendor_error)
{
    switch (vendor_error) {
    case VENDOR_ERR_TIMEOUT:     return -ETIMEDOUT;
    case VENDOR_ERR_PIPE:        return -EPIPE;
    case VENDOR_ERR_NO_DEVICE:   return -ENODEV;
    case VENDOR_ERR_BUSY:        return -EBUSY;
    case VENDOR_ERR_OVERFLOW:    return -EOVERFLOW;
    default:                     return -EIO;
    }
}
```

## Checklist for Porting

- [ ] Identify all USB endpoints used
- [ ] Map transfer types (bulk, control, interrupt, isochronous)
- [ ] Replace blocking calls with `usb_bulk_msg()` / `usb_control_msg()`
- [ ] Convert polling to URB callbacks for async
- [ ] Use `kmalloc()` or `usb_alloc_coherent()` for buffers
- [ ] Handle hot-unplug (disconnect callback)
- [ ] Add power management (suspend/resume)
- [ ] Test error paths (stall, timeout, disconnect)

## Summary

| Migration Task | Kernel Solution |
|----------------|-----------------|
| Device open/close | probe/disconnect callbacks |
| Sync bulk transfer | `usb_bulk_msg()` |
| Async bulk transfer | URB + `usb_submit_urb()` |
| Control request | `usb_control_msg()` |
| Interrupt polling | `usb_fill_int_urb()` + callback |
| DMA buffers | `usb_alloc_coherent()` |
| Error handling | Kernel error codes (-EXXX) |

## Further Reading

- [USB Skeleton Driver](https://elixir.bootlin.com/linux/v6.6/source/drivers/usb/usb-skeleton.c) - Reference implementation
- [Writing USB Drivers](https://docs.kernel.org/driver-api/usb/writing_usb_driver.html) - Official guide
- [libusb Documentation](https://libusb.info/) - For understanding source APIs
