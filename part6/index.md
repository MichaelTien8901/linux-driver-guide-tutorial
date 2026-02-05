---
layout: default
title: "Part 6: Device Model and Driver Framework"
nav_order: 7
has_children: true
---

# Part 6: Device Model and Driver Framework

The Linux device model provides a unified framework for representing devices, their drivers, and their relationships. Understanding this model is essential for writing maintainable, well-integrated drivers.

## The Device Model Hierarchy

```mermaid
flowchart TB
    subgraph Buses["Bus Types"]
        PCI["PCI Bus"]
        USB["USB Bus"]
        Platform["Platform Bus"]
        I2C["I2C Bus"]
    end

    subgraph Devices["Devices"]
        D1["Device 1"]
        D2["Device 2"]
        D3["Device 3"]
    end

    subgraph Drivers["Drivers"]
        Dr1["Driver A"]
        Dr2["Driver B"]
    end

    PCI --> D1
    Platform --> D2
    I2C --> D3

    D1 --> Dr1
    D2 --> Dr1
    D3 --> Dr2

    style Buses fill:#738f99,stroke:#0277bd
    style Devices fill:#8f8a73,stroke:#f9a825
    style Drivers fill:#8f7392,stroke:#6a1b9a
```

## Chapter Contents

| Chapter | Topic | Key Concepts |
|---------|-------|--------------|
| [6.1]({% link part6/01-device-model.md %}) | Device Model | Buses, devices, drivers hierarchy |
| [6.2]({% link part6/02-kobjects.md %}) | Kobjects | kobject, kset, reference counting |
| [6.3]({% link part6/03-platform-drivers.md %}) | Platform Drivers | platform_device, platform_driver |
| [6.4]({% link part6/04-probe-remove.md %}) | Probe and Remove | Driver lifecycle, binding |
| [6.5]({% link part6/05-devres.md %}) | Managed Resources | devm_* functions, automatic cleanup |
| [6.6]({% link part6/06-device-attributes.md %}) | Device Attributes | sysfs interface, DEVICE_ATTR |
| [6.7]({% link part6/07-deferred-probe.md %}) | Deferred Probe | -EPROBE_DEFER handling |

## Key Concepts

### The Bus-Device-Driver Triangle

```mermaid
flowchart LR
    Bus["Bus"]
    Device["Device"]
    Driver["Driver"]

    Bus -->|"has"| Device
    Bus -->|"has"| Driver
    Device <-->|"match & bind"| Driver

    style Bus fill:#738f99,stroke:#0277bd
    style Device fill:#8f8a73,stroke:#f9a825
    style Driver fill:#8f7392,stroke:#6a1b9a
```

1. **Bus**: Defines how devices are discovered and how drivers are matched
2. **Device**: Represents a piece of hardware
3. **Driver**: Contains code to control a device

### Driver Registration Flow

```mermaid
sequenceDiagram
    participant Driver
    participant Bus
    participant Device

    Driver->>Bus: Register driver
    Bus->>Bus: For each device
    Bus->>Bus: Try match(device, driver)
    Bus-->>Driver: Match found!
    Bus->>Driver: Call probe()
    Driver->>Device: Initialize device
```

## Why Platform Drivers?

Not all devices sit on discoverable buses (like PCI or USB). For:

- System-on-Chip (SoC) peripherals
- Memory-mapped devices
- Devices described in Device Tree

We use the **platform bus** - a virtual bus for non-discoverable devices.

## Examples

This part includes working examples:

- **platform-driver**: Complete platform driver implementation

## Prerequisites

Before starting this part, ensure you understand:

- Module lifecycle (Part 2)
- Character device basics (Part 3)
- Memory allocation and mapping (Part 5)

## Further Reading

- [Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html) - Device model documentation
- [Platform Devices](https://docs.kernel.org/driver-api/driver-model/platform.html) - Platform bus guide
- [Devres](https://docs.kernel.org/driver-api/driver-model/devres.html) - Managed device resources

## Next

Start with [Device Model Basics]({% link part6/01-device-model.md %}) to understand the kernel's unified device framework.
