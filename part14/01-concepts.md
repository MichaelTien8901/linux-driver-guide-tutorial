---
layout: default
title: "14.1 USB Concepts"
parent: "Part 14: USB Drivers"
nav_order: 1
---

# USB Concepts

Understand these USB fundamentals: devices, interfaces, endpoints, and URBs.

## Device Identification

USB devices are identified by:
- **Vendor ID (VID)**: Who made it
- **Product ID (PID)**: What model
- **Class/Subclass/Protocol**: What type (HID, storage, etc.)

```c
/* Match by VID:PID */
static const struct usb_device_id my_id_table[] = {
    { USB_DEVICE(0x1234, 0x5678) },
    { }  /* Terminator */
};
MODULE_DEVICE_TABLE(usb, my_id_table);
```

## Interfaces and Endpoints

A USB device has:
- **Configuration**: Power and interface selection (usually one)
- **Interface**: Logical function (your driver binds here)
- **Endpoint**: Data pipe (IN = device→host, OUT = host→device)

```
Device (VID:PID)
└── Configuration
    ├── Interface 0 (your driver binds here)
    │   ├── Endpoint 1 IN  (bulk, for reading)
    │   └── Endpoint 2 OUT (bulk, for writing)
    └── Interface 1 (another driver)
```

{: .note }
> Endpoint 0 is always the control endpoint, used for device setup. You rarely interact with it directly.

## Finding Endpoints

In your probe function, find the endpoints you need:

```c
static int my_probe(struct usb_interface *intf,
                    const struct usb_device_id *id)
{
    struct usb_host_interface *iface_desc = intf->cur_altsetting;
    struct usb_endpoint_descriptor *endpoint;
    int i;

    /* Scan endpoints */
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        endpoint = &iface_desc->endpoint[i].desc;

        if (usb_endpoint_is_bulk_in(endpoint))
            bulk_in_addr = endpoint->bEndpointAddress;
        else if (usb_endpoint_is_bulk_out(endpoint))
            bulk_out_addr = endpoint->bEndpointAddress;
    }
}
```

Helper macros:
- `usb_endpoint_is_bulk_in(ep)`
- `usb_endpoint_is_bulk_out(ep)`
- `usb_endpoint_is_int_in(ep)`
- `usb_endpoint_is_int_out(ep)`

## URB: USB Request Block

URBs are how you communicate with USB devices. Think of them as "USB packets":

```mermaid
flowchart LR
    A["Allocate URB"] --> B["Fill URB"]
    B --> C["Submit URB"]
    C --> D["Wait/Callback"]
    D --> E["Check Status"]
    E --> F["Free/Reuse URB"]

    style C fill:#8f8a73,stroke:#f9a825
```

**Lifecycle:**
1. Allocate: `usb_alloc_urb()`
2. Fill: `usb_fill_bulk_urb()` or similar
3. Submit: `usb_submit_urb()`
4. Completion: Callback called or `usb_wait_urb()`
5. Free: `usb_free_urb()`

## Synchronous vs Asynchronous

**Synchronous** (blocking) - simpler, use for occasional transfers:

```c
/* Blocking bulk transfer */
int ret = usb_bulk_msg(udev, pipe, buffer, len, &actual, timeout);
```

**Asynchronous** (non-blocking) - better for continuous I/O:

```c
/* Non-blocking with callback */
usb_fill_bulk_urb(urb, udev, pipe, buffer, len, callback, context);
usb_submit_urb(urb, GFP_KERNEL);

/* Callback called when complete */
static void my_callback(struct urb *urb)
{
    if (urb->status == 0)
        /* Success: urb->actual_length bytes transferred */
    else
        /* Error handling */
}
```

## Pipes

A "pipe" encodes endpoint address + direction + type:

```c
struct usb_device *udev = interface_to_usbdev(intf);

/* Create pipes */
unsigned int bulk_in_pipe = usb_rcvbulkpipe(udev, bulk_in_addr);
unsigned int bulk_out_pipe = usb_sndbulkpipe(udev, bulk_out_addr);
unsigned int int_in_pipe = usb_rcvintpipe(udev, int_in_addr);
unsigned int ctrl_pipe = usb_sndctrlpipe(udev, 0);
```

## Summary

| Concept | Description |
|---------|-------------|
| VID:PID | Device identification |
| Interface | What your driver binds to |
| Endpoint | Data channel (IN/OUT, bulk/int/iso) |
| URB | Request structure for transfers |
| Pipe | Encoded endpoint for API calls |

## Further Reading

- [USB API](https://docs.kernel.org/driver-api/usb/usb.html) - Core API reference
- [URB Documentation](https://docs.kernel.org/driver-api/usb/URB.html) - URB details
