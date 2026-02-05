# USB Device Demo Driver

A simple USB device driver demonstrating bulk transfers.

## What This Example Shows

- USB driver registration with `module_usb_driver()`
- Device matching by VID:PID
- Endpoint discovery in probe
- Bulk transfers using `usb_bulk_msg()`
- Misc device interface for userspace
- Proper disconnect handling

## Adapting for Your Device

1. Find your device's VID:PID:
   ```bash
   lsusb
   # Output: Bus 001 Device 005: ID 1234:5678 My Device
   ```

2. Modify the header in `usb_demo.c`:
   ```c
   #define DEMO_VID 0x1234
   #define DEMO_PID 0x5678
   ```

3. Rebuild and load.

## Building

```bash
make
```

## Testing

```bash
# Load module
sudo insmod usb_demo.ko

# Check if device was detected (dmesg)
dmesg | tail

# If device attached, /dev/usb_demo should exist
ls -la /dev/usb_demo

# Write to device
echo "Hello USB" | sudo tee /dev/usb_demo

# Read from device
sudo cat /dev/usb_demo

# Unload
sudo rmmod usb_demo
```

## Testing Without Hardware

For testing without a real device, you can use the kernel's dummy_hcd and gadgetfs to create a virtual USB device. This is advanced and requires gadget configuration.

## Key Code Sections

### Device Matching

```c
static const struct usb_device_id usb_demo_id_table[] = {
    { USB_DEVICE(DEMO_VID, DEMO_PID) },
    { }
};
```

### Endpoint Discovery

```c
static int usb_demo_probe(struct usb_interface *intf, ...)
{
    struct usb_host_interface *iface_desc = intf->cur_altsetting;

    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        ep = &iface_desc->endpoint[i].desc;

        if (usb_endpoint_is_bulk_in(ep))
            dev->bulk_in_addr = ep->bEndpointAddress;
        if (usb_endpoint_is_bulk_out(ep))
            dev->bulk_out_addr = ep->bEndpointAddress;
    }
}
```

### Bulk Transfer

```c
ret = usb_bulk_msg(
    dev->udev,
    usb_rcvbulkpipe(dev->udev, dev->bulk_in_addr),
    buffer,
    count,
    &actual_len,
    5000  /* timeout ms */
);
```

## Files

- `usb_demo.c` - Complete driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- [Part 14.1: USB Concepts](../../../part14/01-concepts.md)
- [Part 14.2: Driver Skeleton](../../../part14/02-driver-skeleton.md)
- [Part 14.3: USB Transfers](../../../part14/03-transfers.md)

## Further Reading

- [USB API](https://docs.kernel.org/driver-api/usb/index.html)
- [Writing USB Drivers](https://docs.kernel.org/driver-api/usb/writing_usb_driver.html)
