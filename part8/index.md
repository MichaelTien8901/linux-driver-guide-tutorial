---
layout: default
title: "Part 8: Platform Bus and Device Tree"
nav_order: 9
has_children: true
---

# Part 8: Platform Bus and Device Tree

Device Tree is the standard mechanism for describing hardware in embedded Linux systems. Understanding Device Tree is essential for writing portable, maintainable drivers.

## Why Device Tree?

```mermaid
flowchart LR
    subgraph Old["Without Device Tree"]
        direction LR
        BOARD[Board File]
        CODE["Hardcoded in C"]
        DIFF["Different code per board"]
    end

    subgraph New["With Device Tree"]
        direction LR
        DTS[Device Tree Source]
        DTB[Device Tree Blob]
        SAME["Same driver, different DT"]
    end

    BOARD --> CODE --> DIFF
    DTS --> DTB --> SAME

    style Old fill:#8f6d72,stroke:#c62828
    style New fill:#486649,stroke:#2e7d32
```

## Chapter Contents

| Chapter | Topic | Key Concepts |
|---------|-------|--------------|
| [8.1]({% link part8/01-devicetree-basics.md %}) | Device Tree Basics | DTS syntax, nodes, properties |
| [8.2]({% link part8/02-bindings.md %}) | Device Bindings | compatible, of_match_table |
| [8.3]({% link part8/03-property-access.md %}) | Property Access | of_property_read_*, device_property_* |
| [8.4]({% link part8/04-gpio-references.md %}) | GPIO in Device Tree | GPIO specifiers, gpiod_get |
| [8.5]({% link part8/05-clock-regulator-refs.md %}) | Clocks and Regulators | Clock and regulator references |
| [8.6]({% link part8/06-overlays.md %}) | Device Tree Overlays | Runtime DT modification |
| [8.7]({% link part8/07-binding-docs.md %}) | Binding Documentation | YAML bindings, dt-schema |

## Device Tree Overview

```mermaid
flowchart LR
    subgraph Source["Source Files"]
        DTS[".dts file<br/>(board-specific)"]
        DTSI[".dtsi files<br/>(SoC, includes)"]
    end

    subgraph Compile["Compilation"]
        DTC["dtc compiler"]
    end

    subgraph Output["Output"]
        DTB[".dtb blob<br/>(binary)"]
    end

    subgraph Boot["Boot"]
        BL["Bootloader"]
        KERN["Kernel"]
    end

    DTS --> DTC
    DTSI --> DTS
    DTC --> DTB
    DTB --> BL
    BL -->|"Pass to"| KERN
```

## Basic DTS Structure

```dts
/dts-v1/;

/ {
    compatible = "vendor,board";
    model = "Vendor Board Name";

    /* CPU cluster */
    cpus {
        #address-cells = <1>;
        #size-cells = <0>;

        cpu@0 {
            compatible = "arm,cortex-a53";
            device_type = "cpu";
            reg = <0>;
        };
    };

    /* Memory */
    memory@80000000 {
        device_type = "memory";
        reg = <0x80000000 0x40000000>;
    };

    /* SoC peripherals */
    soc {
        compatible = "simple-bus";
        #address-cells = <1>;
        #size-cells = <1>;
        ranges;

        uart0: serial@10000000 {
            compatible = "vendor,uart";
            reg = <0x10000000 0x1000>;
            interrupts = <0 10 4>;
            clocks = <&clk_uart>;
            status = "okay";
        };

        gpio0: gpio@10001000 {
            compatible = "vendor,gpio";
            reg = <0x10001000 0x1000>;
            gpio-controller;
            #gpio-cells = <2>;
        };
    };
};
```

## Key Concepts

### Node Addressing

```mermaid
flowchart TB
    subgraph Node["Node Name: unit@address"]
        NAME["uart0: serial@10000000"]
        LABEL["uart0 = label (phandle)"]
        UNIT["serial = unit name"]
        ADDR["10000000 = unit address"]
    end

    NAME --> LABEL
    NAME --> UNIT
    NAME --> ADDR
```

### Properties

| Property | Purpose | Example |
|----------|---------|---------|
| `compatible` | Identifies device/driver | `"vendor,device"` |
| `reg` | Memory/IO addresses | `<0x10000 0x1000>` |
| `interrupts` | Interrupt specifiers | `<0 15 4>` |
| `clocks` | Clock references | `<&clk 0>` |
| `status` | Enable/disable | `"okay"` or `"disabled"` |

### Driver Matching

```mermaid
sequenceDiagram
    participant DT as Device Tree
    participant BUS as Platform Bus
    participant DRV as Driver

    DT->>BUS: Create devices from DT nodes
    BUS->>BUS: For each platform_device

    DRV->>BUS: Register with of_match_table

    BUS->>BUS: Match compatible strings
    BUS-->>DRV: Found match!

    BUS->>DRV: Call probe()
    DRV->>DT: Read properties
```

## Property Types

```c
/* String */
compatible = "vendor,device";

/* String list */
compatible = "vendor,device-v2", "vendor,device";

/* 32-bit integer */
reg-shift = <2>;

/* Integer array */
reg = <0x10000000 0x1000>;

/* phandle reference */
clocks = <&clk_main>;

/* phandle with arguments */
clocks = <&clk_provider 5>;

/* Boolean (presence = true) */
big-endian;
```

## Examples

This part includes working examples:

- **dt-platform-driver**: Platform driver with Device Tree binding

## Prerequisites

Before starting this part, ensure you understand:

- Platform drivers (Part 6)
- Probe and remove lifecycle (Part 6)
- Interrupt handling basics (Part 7)

## Further Reading

- [Device Tree Specification](https://devicetree.org/) - Official DT specification
- [Devicetree Usage](https://docs.kernel.org/devicetree/usage-model.html) - Kernel DT documentation
- [Binding Index](https://docs.kernel.org/devicetree/bindings/index.html) - Device binding reference

## Next

Start with [Device Tree Basics]({% link part8/01-devicetree-basics.md %}) to learn DTS syntax and structure.
