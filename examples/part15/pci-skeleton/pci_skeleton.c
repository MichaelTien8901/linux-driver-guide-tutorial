// SPDX-License-Identifier: GPL-2.0
/*
 * PCI Driver Skeleton
 *
 * A minimal PCI driver template demonstrating:
 * - Device matching by VID:PID
 * - BAR mapping with managed resources
 * - MSI interrupt setup
 * - Proper error handling
 *
 * NOTE: Modify VID:PID to match your actual device.
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>

#define DRIVER_NAME "pci_skeleton"

/* Change these to match your device */
#define SKELETON_VID 0x1234
#define SKELETON_PID 0x5678

/* Example register offsets (device-specific) */
#define REG_STATUS    0x00
#define REG_CONTROL   0x04
#define REG_DATA      0x08

struct skeleton_dev {
    struct pci_dev *pdev;
    void __iomem *regs;
    int irq;

    /* Device state */
    u32 hw_version;
};

/* ============ Hardware Access ============ */

static inline u32 skel_read(struct skeleton_dev *dev, int reg)
{
    return readl(dev->regs + reg);
}

static inline void skel_write(struct skeleton_dev *dev, int reg, u32 val)
{
    writel(val, dev->regs + reg);
}

/* ============ Interrupt Handler ============ */

static irqreturn_t skeleton_irq(int irq, void *data)
{
    struct skeleton_dev *dev = data;
    u32 status;

    status = skel_read(dev, REG_STATUS);

    /* Check if this interrupt is from our device */
    if (!(status & 0x1))
        return IRQ_NONE;

    /* Acknowledge/clear interrupt */
    skel_write(dev, REG_STATUS, status);

    dev_dbg(&dev->pdev->dev, "Interrupt handled, status=0x%08x\n", status);

    return IRQ_HANDLED;
}

/* ============ Device Initialization ============ */

static int skeleton_hw_init(struct skeleton_dev *dev)
{
    /* Read hardware version (example) */
    dev->hw_version = skel_read(dev, REG_STATUS);

    dev_info(&dev->pdev->dev, "Hardware version: 0x%08x\n", dev->hw_version);

    /* Initialize hardware (device-specific) */
    /* skel_write(dev, REG_CONTROL, ...); */

    return 0;
}

/* ============ Probe/Remove ============ */

static int skeleton_probe(struct pci_dev *pdev,
                          const struct pci_device_id *id)
{
    struct skeleton_dev *dev;
    int ret;

    dev_info(&pdev->dev, "Probing PCI device %04x:%04x\n",
             pdev->vendor, pdev->device);

    /* Allocate device structure */
    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->pdev = pdev;
    pci_set_drvdata(pdev, dev);

    /* Enable PCI device */
    ret = pcim_enable_device(pdev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable PCI device: %d\n", ret);
        return ret;
    }

    /* Request and map BAR0 */
    ret = pcim_iomap_regions(pdev, BIT(0), DRIVER_NAME);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request regions: %d\n", ret);
        return ret;
    }

    dev->regs = pcim_iomap_table(pdev)[0];
    if (!dev->regs) {
        dev_err(&pdev->dev, "Failed to map BAR0\n");
        return -ENOMEM;
    }

    dev_info(&pdev->dev, "BAR0 mapped at %p (size: %llu)\n",
             dev->regs, pci_resource_len(pdev, 0));

    /* Enable bus mastering (needed for DMA) */
    pci_set_master(pdev);

    /* Set DMA mask - try 64-bit first, fall back to 32-bit */
    ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
    if (ret) {
        ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
        if (ret) {
            dev_err(&pdev->dev, "Failed to set DMA mask: %d\n", ret);
            return ret;
        }
        dev_info(&pdev->dev, "Using 32-bit DMA\n");
    } else {
        dev_info(&pdev->dev, "Using 64-bit DMA\n");
    }

    /* Allocate MSI interrupt */
    ret = pci_alloc_irq_vectors(pdev, 1, 1,
                                PCI_IRQ_MSI | PCI_IRQ_LEGACY);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to allocate IRQ: %d\n", ret);
        return ret;
    }

    dev->irq = pci_irq_vector(pdev, 0);

    ret = devm_request_irq(&pdev->dev, dev->irq, skeleton_irq,
                           0, DRIVER_NAME, dev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ %d: %d\n",
                dev->irq, ret);
        goto free_irq_vectors;
    }

    dev_info(&pdev->dev, "IRQ %d allocated\n", dev->irq);

    /* Initialize hardware */
    ret = skeleton_hw_init(dev);
    if (ret)
        goto free_irq_vectors;

    dev_info(&pdev->dev, "PCI skeleton driver loaded\n");
    return 0;

free_irq_vectors:
    pci_free_irq_vectors(pdev);
    return ret;
}

static void skeleton_remove(struct pci_dev *pdev)
{
    struct skeleton_dev *dev = pci_get_drvdata(pdev);

    /* Disable hardware interrupts first (device-specific) */
    /* skel_write(dev, REG_CONTROL, 0); */

    pci_free_irq_vectors(pdev);

    dev_info(&pdev->dev, "PCI skeleton driver removed\n");
}

/* ============ Device ID Table ============ */

static const struct pci_device_id skeleton_ids[] = {
    { PCI_DEVICE(SKELETON_VID, SKELETON_PID) },
    { }
};
MODULE_DEVICE_TABLE(pci, skeleton_ids);

/* ============ Driver Structure ============ */

static struct pci_driver skeleton_driver = {
    .name = DRIVER_NAME,
    .id_table = skeleton_ids,
    .probe = skeleton_probe,
    .remove = skeleton_remove,
};

module_pci_driver(skeleton_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("PCI Driver Skeleton");
