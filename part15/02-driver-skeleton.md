---
layout: default
title: "15.2 Driver Skeleton"
parent: "Part 15: PCIe Drivers"
nav_order: 2
---

# PCI Driver Skeleton

This chapter shows the standard PCI driver structure.

## Complete Skeleton

```c
#include <linux/module.h>
#include <linux/pci.h>

#define DRIVER_NAME "my_pci"

struct my_pci_dev {
    struct pci_dev *pdev;
    void __iomem *regs;    /* Mapped BAR0 */
    int irq;
};

/* ============ Hardware Access ============ */

static u32 my_read_reg(struct my_pci_dev *dev, int offset)
{
    return readl(dev->regs + offset);
}

static void my_write_reg(struct my_pci_dev *dev, int offset, u32 val)
{
    writel(val, dev->regs + offset);
}

/* ============ Interrupt Handler ============ */

static irqreturn_t my_irq_handler(int irq, void *data)
{
    struct my_pci_dev *dev = data;

    /* Check if interrupt is ours */
    u32 status = my_read_reg(dev, 0x00);  /* Status register */
    if (!(status & 0x1))
        return IRQ_NONE;

    /* Clear interrupt */
    my_write_reg(dev, 0x00, status);

    /* Handle interrupt... */

    return IRQ_HANDLED;
}

/* ============ Probe/Remove ============ */

static int my_pci_probe(struct pci_dev *pdev,
                        const struct pci_device_id *id)
{
    struct my_pci_dev *dev;
    int ret;

    /* Allocate driver data */
    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->pdev = pdev;
    pci_set_drvdata(pdev, dev);

    /* Enable device */
    ret = pcim_enable_device(pdev);
    if (ret)
        return ret;

    /* Request regions */
    ret = pcim_iomap_regions(pdev, BIT(0), DRIVER_NAME);
    if (ret)
        return ret;

    /* Map BAR0 */
    dev->regs = pcim_iomap_table(pdev)[0];
    if (!dev->regs)
        return -ENOMEM;

    /* Enable bus mastering (for DMA) */
    pci_set_master(pdev);

    /* Set DMA mask */
    ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
    if (ret) {
        ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
        if (ret)
            return ret;
    }

    /* Allocate MSI interrupt */
    ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI | PCI_IRQ_LEGACY);
    if (ret < 0)
        return ret;

    dev->irq = pci_irq_vector(pdev, 0);

    ret = devm_request_irq(&pdev->dev, dev->irq, my_irq_handler,
                           0, DRIVER_NAME, dev);
    if (ret)
        goto free_irq_vectors;

    dev_info(&pdev->dev, "PCI device probed (BAR0: %pR)\n",
             &pdev->resource[0]);

    return 0;

free_irq_vectors:
    pci_free_irq_vectors(pdev);
    return ret;
}

static void my_pci_remove(struct pci_dev *pdev)
{
    pci_free_irq_vectors(pdev);
    dev_info(&pdev->dev, "PCI device removed\n");
}

/* ============ Device ID Table ============ */

static const struct pci_device_id my_pci_ids[] = {
    { PCI_DEVICE(0x1234, 0x5678) },  /* Your device */
    { }
};
MODULE_DEVICE_TABLE(pci, my_pci_ids);

/* ============ Driver Structure ============ */

static struct pci_driver my_pci_driver = {
    .name = DRIVER_NAME,
    .id_table = my_pci_ids,
    .probe = my_pci_probe,
    .remove = my_pci_remove,
};

module_pci_driver(my_pci_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("My PCI Driver");
```

## Key Steps in Probe

### 1. Enable Device

```c
/* Managed version - auto cleanup on remove */
ret = pcim_enable_device(pdev);

/* Or manual version */
ret = pci_enable_device(pdev);
/* Must call pci_disable_device() in remove */
```

### 2. Request and Map BARs

```c
/* Managed version */
ret = pcim_iomap_regions(pdev, BIT(0), "driver_name");
void __iomem *regs = pcim_iomap_table(pdev)[0];

/* Or manual version */
ret = pci_request_region(pdev, 0, "driver_name");
void __iomem *regs = pci_iomap(pdev, 0, 0);
/* Must call pci_iounmap() and pci_release_region() in remove */
```

### 3. Set Up DMA

```c
/* Enable bus mastering */
pci_set_master(pdev);

/* Set DMA mask */
ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
```

### 4. Set Up Interrupts

```c
/* Allocate MSI/MSI-X or fall back to legacy */
ret = pci_alloc_irq_vectors(pdev, 1, 1,
                            PCI_IRQ_MSI | PCI_IRQ_MSIX | PCI_IRQ_LEGACY);

int irq = pci_irq_vector(pdev, 0);
ret = request_irq(irq, handler, 0, "driver", dev);
```

## Managed vs Manual Resources

Use managed (`pcim_*`, `devm_*`) functions when possible:

| Managed | Manual | Notes |
|---------|--------|-------|
| `pcim_enable_device()` | `pci_enable_device()` | Auto-disabled |
| `pcim_iomap_regions()` | `pci_request_region()` + `pci_iomap()` | Auto-unmapped |
| `devm_request_irq()` | `request_irq()` | Auto-freed |

{: .tip }
> Managed resources are released automatically in reverse order when probe fails or remove is called.

## Reading Device Info

```bash
# Find your device
lspci -nn

# Detailed view
lspci -v -s 00:1f.6

# See BAR mappings
lspci -v -s 00:1f.6 | grep -i memory
```

## Summary

| Step | Function |
|------|----------|
| Enable | `pcim_enable_device()` |
| Request BARs | `pcim_iomap_regions()` |
| Map BARs | `pcim_iomap_table()` |
| DMA setup | `pci_set_master()`, `dma_set_mask()` |
| Interrupts | `pci_alloc_irq_vectors()`, `pci_irq_vector()` |

## Further Reading

- [PCI Driver API](https://docs.kernel.org/PCI/pci.html) - Complete reference
- [MSI HOWTO](https://docs.kernel.org/PCI/msi-howto.html) - Interrupt setup
