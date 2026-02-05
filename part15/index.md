---
layout: default
title: "Part 15: PCIe Drivers"
nav_order: 15
has_children: true
---

# Part 15: PCIe Drivers

PCI/PCIe drivers control expansion cards and on-board peripherals connected via the PCI bus.

## PCI Model

PCI devices are identified by:
- **Vendor ID**: Who made it (assigned by PCI-SIG)
- **Device ID**: What model
- **Class**: What type (network, storage, etc.)

```mermaid
flowchart TB
    subgraph System["System"]
        CPU["CPU"]
        MEM["Memory"]
    end

    subgraph PCIe["PCIe Root Complex"]
        RC["Root Complex"]
        SW["PCIe Switch"]
    end

    subgraph Devices["PCI Devices"]
        NIC["Network Card<br/>BAR0: Registers<br/>BAR1: DMA"]
        GPU["Graphics Card<br/>BAR0: MMIO<br/>BAR2: VRAM"]
        SSD["NVMe SSD<br/>BAR0: Registers"]
    end

    CPU --> RC
    MEM --> RC
    RC --> SW
    SW --> NIC
    SW --> GPU
    RC --> SSD

    style System fill:#738f99,stroke:#0277bd
    style PCIe fill:#7a8f73,stroke:#2e7d32
    style Devices fill:#8f8a73,stroke:#f9a825
```

## BARs: Base Address Registers

Each PCI device has up to 6 BARs (Base Address Registers) that define memory or I/O regions:

| BAR Type | Access | Use Case |
|----------|--------|----------|
| Memory-mapped | `ioremap()` | Registers, DMA buffers |
| I/O ports | `inb()`/`outb()` | Legacy devices (rare now) |

Your driver maps BARs to access device registers.

## Key Concepts

- **pci_dev**: Represents the PCI device
- **pci_driver**: Your driver, with probe/remove
- **BARs**: Memory regions to map
- **MSI/MSI-X**: Modern interrupt mechanism
- **DMA**: Direct memory access

## Chapters

| Chapter | What You'll Learn |
|---------|-------------------|
| [Concepts]({% link part15/01-concepts.md %}) | PCI model, BARs, configuration space |
| [Driver Skeleton]({% link part15/02-driver-skeleton.md %}) | pci_driver structure and resource mapping |
| [MSI/MSI-X Interrupts]({% link part15/03-msi-interrupts.md %}) | Multiple vectors, interrupt affinity |
| [DMA Operations]({% link part15/04-dma-operations.md %}) | Coherent and streaming DMA mappings |

## Example

- **[PCI Skeleton](../examples/part15/pci-skeleton/)** - Minimal PCI driver template

## Prerequisites

- Device model (Part 6)
- [Managed resources (devm_*)]({% link part6/05-devres.md %}) - PCI uses pcim_* and devm_* extensively
- Memory-mapped I/O (Part 5)
- Interrupt handling (Part 7)

## Further Reading

- [PCI Documentation](https://docs.kernel.org/PCI/index.html) - Kernel docs
- [PCI Driver API](https://docs.kernel.org/PCI/pci.html) - Function reference
- [MSI-HOWTO](https://docs.kernel.org/PCI/msi-howto.html) - Interrupt setup
