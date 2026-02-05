---
layout: default
title: "15.1 PCI Concepts"
parent: "Part 15: PCIe Drivers"
nav_order: 1
---

# PCI Concepts

Understand PCI device identification, BARs, and configuration space.

## Device Identification

Every PCI device has:
- **Vendor ID** (16-bit): Manufacturer
- **Device ID** (16-bit): Specific product
- **Class code**: Device type (network, storage, etc.)

```c
static const struct pci_device_id my_pci_ids[] = {
    /* Match by vendor/device */
    { PCI_DEVICE(0x1234, 0x5678) },

    /* Match by class */
    { PCI_DEVICE_CLASS(PCI_CLASS_NETWORK_ETHERNET << 8, 0xffff00) },

    { }  /* Terminator */
};
MODULE_DEVICE_TABLE(pci, my_pci_ids);
```

Find device IDs with `lspci -nn`:
```bash
$ lspci -nn
00:1f.6 Ethernet [0200]: Intel [8086:15d8] (rev 10)
#       Class              Vendor  Device
```

## BARs: Base Address Registers

PCI devices expose memory regions via BARs:

```
Device Configuration Space
├── BAR0: 0xfe000000 (64KB memory-mapped)
├── BAR1: 0xfdf00000 (1MB memory-mapped)
├── BAR2: 0x3000     (I/O ports, rare)
└── ...
```

**In your driver:**

```c
/* Get BAR info */
resource_size_t start = pci_resource_start(pdev, 0);
resource_size_t len = pci_resource_len(pdev, 0);
unsigned long flags = pci_resource_flags(pdev, 0);

if (flags & IORESOURCE_MEM) {
    /* Memory-mapped BAR - use ioremap */
    void __iomem *regs = pci_iomap(pdev, 0, len);
    /* Access: readl(regs + OFFSET) */
}
```

## Configuration Space

Each PCI device has 256 bytes (PCIe: 4KB) of configuration space:

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 2 | Vendor ID |
| 0x02 | 2 | Device ID |
| 0x04 | 2 | Command |
| 0x06 | 2 | Status |
| 0x10-0x24 | 24 | BAR0-BAR5 |
| ... | ... | ... |

**Access configuration space:**

```c
u16 vendor, device;
pci_read_config_word(pdev, PCI_VENDOR_ID, &vendor);
pci_read_config_word(pdev, PCI_DEVICE_ID, &device);

/* Write (careful!) */
pci_write_config_word(pdev, PCI_COMMAND, cmd);
```

## Enable the Device

Before using a PCI device:

```c
/* 1. Enable device (memory/IO access) */
ret = pci_enable_device(pdev);

/* 2. Request BAR regions */
ret = pci_request_regions(pdev, "my_driver");

/* 3. Set DMA mask if needed */
ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));

/* 4. Enable bus mastering for DMA */
pci_set_master(pdev);
```

## MSI/MSI-X Interrupts

Modern PCI devices use MSI (Message Signaled Interrupts):

```c
/* Enable MSI (single interrupt) */
ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI);

/* Or MSI-X (multiple interrupts) */
ret = pci_alloc_irq_vectors(pdev, 1, num_vecs, PCI_IRQ_MSIX);

/* Get IRQ number */
int irq = pci_irq_vector(pdev, 0);
ret = request_irq(irq, my_handler, 0, "my_driver", dev);
```

{: .tip }
> Prefer MSI/MSI-X over legacy interrupts. They're faster and avoid sharing issues.

## Summary

| Concept | Description |
|---------|-------------|
| Vendor/Device ID | Unique device identification |
| BAR | Memory region for device access |
| Configuration space | Device settings and capabilities |
| MSI/MSI-X | Modern interrupt mechanism |

## Further Reading

- [PCI API](https://docs.kernel.org/PCI/pci.html) - Function reference
- [PCI Express](https://docs.kernel.org/PCI/pciebus-howto.html) - PCIe specifics
