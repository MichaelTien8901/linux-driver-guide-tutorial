---
layout: default
title: "Part 14: USB Drivers"
nav_order: 14
has_children: true
---

# Part 14: USB Drivers

USB drivers connect the kernel to USB devices. This part covers USB device drivers (not host controller drivers).

## USB Model

USB is hierarchical: Host → Hub(s) → Device → Interface(s) → Endpoint(s)

```mermaid
flowchart TB
    subgraph Host["USB Host"]
        HC["Host Controller"]
    end

    subgraph Device["USB Device"]
        DEV["Device<br/>(VID:PID)"]
        CFG["Configuration"]
        IF1["Interface 0<br/>(class/subclass)"]
        IF2["Interface 1"]
        EP1["EP1 IN<br/>(Bulk)"]
        EP2["EP2 OUT<br/>(Bulk)"]
        EP3["EP0<br/>(Control)"]
    end

    HC --> DEV
    DEV --> CFG
    CFG --> IF1
    CFG --> IF2
    IF1 --> EP1
    IF1 --> EP2
    DEV --> EP3

    style Host fill:#738f99,stroke:#0277bd
    style Device fill:#8f8a73,stroke:#f9a825
```

**Key insight**: Your driver binds to an **interface**, not the whole device. A USB device can have multiple interfaces (e.g., keyboard + media keys).

## Transfer Types

| Type | Use Case | Characteristics |
|------|----------|-----------------|
| **Control** | Configuration, commands | Guaranteed delivery, bidirectional |
| **Bulk** | Data transfer (storage) | Reliable, no bandwidth guarantee |
| **Interrupt** | Small periodic data (HID) | Guaranteed latency, small packets |
| **Isochronous** | Streaming (audio/video) | Guaranteed bandwidth, no retry |

Most drivers use **bulk** (data) or **interrupt** (events) transfers.

## Linux USB Architecture

```mermaid
flowchart TB
    subgraph Drivers["USB Drivers"]
        YOUR["Your Driver"]
        HID["HID Driver"]
        STORAGE["Storage Driver"]
    end

    subgraph Core["USB Core"]
        USBCORE["USB Core"]
        USBBUS["USB Bus"]
    end

    subgraph HCD["Host Controller"]
        EHCI["EHCI/XHCI"]
        HW["USB Hardware"]
    end

    YOUR --> USBCORE
    HID --> USBCORE
    STORAGE --> USBCORE
    USBCORE --> USBBUS
    USBBUS --> EHCI
    EHCI --> HW

    style Drivers fill:#7a8f73,stroke:#2e7d32
    style Core fill:#738f99,stroke:#0277bd
    style HCD fill:#8f8a73,stroke:#f9a825
```

## Chapters

| Chapter | What You'll Learn |
|---------|-------------------|
| [Concepts]({% link part14/01-concepts.md %}) | USB model, endpoints, and URBs |
| [Driver Skeleton]({% link part14/02-driver-skeleton.md %}) | usb_driver structure and probe/disconnect |
| [Transfers]({% link part14/03-transfers.md %}) | Control, bulk, and interrupt URBs |
| [Error Handling]({% link part14/04-error-handling.md %}) | Disconnect races, URB errors, recovery |
| [Suspend and Resume]({% link part14/05-suspend-resume.md %}) | Autosuspend, PM callbacks |
| [User Space Interface]({% link part14/06-user-interface.md %}) | Character device, file operations, ioctl |
| [Porting Guide]({% link part14/07-porting-guide.md %}) | Migrating from libusb, vendor SDKs |
| [WinUSB and libusb]({% link part14/08-winusb-libusb.md %}) | Cross-platform compatibility, usbfs |
| [USB Class Drivers]({% link part14/09-usb-class-drivers.md %}) | ACM, mass storage, network (NCM/RNDIS), MCTP |
| [USB Gadget Drivers]({% link part14/10-usb-gadget.md %}) | Device-side drivers, ConfigFS, FunctionFS |

## Example

- **[USB Device Driver](../examples/part14/usb-device/)** - Bulk transfer example with simple protocol

## Prerequisites

- Kernel module basics (Part 1-2)
- Device model (Part 6) - probe/remove pattern
- [Managed resources (devm_*)]({% link part6/05-devres.md %}) - USB drivers use usb_* and devm_* functions
- Understanding of asynchronous I/O

## Further Reading

- [USB API Documentation](https://docs.kernel.org/driver-api/usb/index.html) - Official reference
- [USB Core API](https://docs.kernel.org/driver-api/usb/usb.html) - Core functions
- [Writing USB Drivers](https://docs.kernel.org/driver-api/usb/writing_usb_driver.html) - Tutorial
- [USB in a Nutshell](https://www.beyondlogic.org/usbnutshell/usb1.shtml) - Protocol basics
