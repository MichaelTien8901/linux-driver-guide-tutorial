---
layout: default
title: "15.3 MSI/MSI-X Interrupts"
parent: "Part 15: PCIe Drivers"
nav_order: 3
---

# MSI/MSI-X Interrupts

This chapter covers advanced interrupt setup: MSI-X with multiple vectors and interrupt affinity.

## Why MSI-X?

| Feature | Legacy IRQ | MSI | MSI-X |
|---------|------------|-----|-------|
| Vectors | 1 (shared) | 1-32 | Up to 2048 |
| Sharing | Required | None | None |
| Steering | None | None | Per-CPU |
| Performance | Low | Good | Best |

**Use MSI-X when:**
- Device has multiple queues (network, storage)
- You want per-CPU interrupt handling
- High-performance I/O is needed

## Basic MSI-X Setup

```c
static int setup_msix(struct my_pci_dev *dev)
{
    int num_vecs, i, ret;

    /* Request as many vectors as queues */
    num_vecs = pci_alloc_irq_vectors(dev->pdev, 1, dev->num_queues,
                                      PCI_IRQ_MSIX | PCI_IRQ_MSI);
    if (num_vecs < 0)
        return num_vecs;

    dev->num_irqs = num_vecs;

    /* Register handler for each vector */
    for (i = 0; i < num_vecs; i++) {
        int irq = pci_irq_vector(dev->pdev, i);

        ret = request_irq(irq, my_irq_handler, 0,
                          dev->irq_names[i], &dev->queues[i]);
        if (ret) {
            /* Cleanup previously registered IRQs */
            while (--i >= 0)
                free_irq(pci_irq_vector(dev->pdev, i), &dev->queues[i]);
            pci_free_irq_vectors(dev->pdev);
            return ret;
        }
    }

    return 0;
}
```

## Per-Queue Interrupt Handlers

Each vector can have its own handler:

```c
struct my_queue {
    struct my_pci_dev *dev;
    int queue_id;
    struct napi_struct napi;  /* For network drivers */
};

static irqreturn_t queue_irq_handler(int irq, void *data)
{
    struct my_queue *queue = data;

    /* Handle this queue's interrupt */
    u32 status = readl(queue->dev->regs + QUEUE_STATUS(queue->queue_id));

    if (!(status & QUEUE_IRQ_PENDING))
        return IRQ_NONE;

    /* Clear interrupt */
    writel(status, queue->dev->regs + QUEUE_STATUS(queue->queue_id));

    /* Process queue... */

    return IRQ_HANDLED;
}
```

## Interrupt Affinity

Bind interrupts to specific CPUs for better cache locality:

```c
static int setup_irq_affinity(struct my_pci_dev *dev)
{
    int i;

    for (i = 0; i < dev->num_irqs; i++) {
        int irq = pci_irq_vector(dev->pdev, i);
        int cpu = i % num_online_cpus();

        /* Set CPU affinity for this IRQ */
        irq_set_affinity_hint(irq, cpumask_of(cpu));
    }

    return 0;
}

static void clear_irq_affinity(struct my_pci_dev *dev)
{
    int i;

    for (i = 0; i < dev->num_irqs; i++) {
        int irq = pci_irq_vector(dev->pdev, i);
        irq_set_affinity_hint(irq, NULL);
    }
}
```

## Managed Interrupt Allocation

Let the kernel choose optimal affinity:

```c
static int setup_managed_irqs(struct my_pci_dev *dev)
{
    struct irq_affinity affd = {
        .pre_vectors = 1,   /* Admin queue */
        .post_vectors = 0,
    };
    int num_vecs;

    /* Kernel automatically spreads interrupts across CPUs */
    num_vecs = pci_alloc_irq_vectors_affinity(dev->pdev,
                                               1, dev->num_queues,
                                               PCI_IRQ_MSIX | PCI_IRQ_AFFINITY,
                                               &affd);
    if (num_vecs < 0)
        return num_vecs;

    /* Use devm_request_irq for automatic cleanup */
    for (int i = 0; i < num_vecs; i++) {
        int irq = pci_irq_vector(dev->pdev, i);
        int ret = devm_request_irq(&dev->pdev->dev, irq,
                                    queue_irq_handler, 0,
                                    dev_name(&dev->pdev->dev),
                                    &dev->queues[i]);
        if (ret)
            return ret;
    }

    return num_vecs;
}
```

## Fallback Strategy

Handle systems without MSI-X support:

```c
static int setup_interrupts(struct my_pci_dev *dev)
{
    int num_vecs;

    /* Try MSI-X first */
    num_vecs = pci_alloc_irq_vectors(dev->pdev, 1, dev->num_queues,
                                      PCI_IRQ_MSIX);
    if (num_vecs > 0) {
        dev_info(&dev->pdev->dev, "Using MSI-X with %d vectors\n", num_vecs);
        return setup_multiple_irqs(dev, num_vecs);
    }

    /* Fall back to MSI */
    num_vecs = pci_alloc_irq_vectors(dev->pdev, 1, 1, PCI_IRQ_MSI);
    if (num_vecs > 0) {
        dev_info(&dev->pdev->dev, "Using MSI\n");
        return setup_single_irq(dev);
    }

    /* Fall back to legacy INTx */
    num_vecs = pci_alloc_irq_vectors(dev->pdev, 1, 1, PCI_IRQ_LEGACY);
    if (num_vecs > 0) {
        dev_info(&dev->pdev->dev, "Using legacy IRQ\n");
        return setup_single_irq(dev);
    }

    return -ENODEV;
}
```

## Shared vs Dedicated Handlers

For single MSI mode with multiple queues:

```c
static irqreturn_t shared_irq_handler(int irq, void *data)
{
    struct my_pci_dev *dev = data;
    u32 status = readl(dev->regs + GLOBAL_IRQ_STATUS);
    irqreturn_t ret = IRQ_NONE;
    int i;

    /* Check each queue */
    for (i = 0; i < dev->num_queues; i++) {
        if (status & BIT(i)) {
            handle_queue_irq(&dev->queues[i]);
            ret = IRQ_HANDLED;
        }
    }

    /* Clear processed interrupts */
    writel(status, dev->regs + GLOBAL_IRQ_STATUS);

    return ret;
}
```

## Interrupt Coalescing

Reduce interrupt rate for high-throughput scenarios:

```c
struct irq_coalesce_params {
    u32 max_frames;    /* Interrupt after N frames */
    u32 max_usecs;     /* Or after N microseconds */
};

static void configure_coalescing(struct my_pci_dev *dev,
                                  struct irq_coalesce_params *params)
{
    /* Hardware-specific coalescing registers */
    writel(params->max_frames, dev->regs + IRQ_COALESCE_FRAMES);
    writel(params->max_usecs, dev->regs + IRQ_COALESCE_TIMEOUT);
}
```

## Complete MSI-X Example

```c
#define MAX_QUEUES 16

struct my_pci_dev {
    struct pci_dev *pdev;
    void __iomem *regs;

    int num_queues;
    int num_irqs;
    struct my_queue queues[MAX_QUEUES];
    char irq_names[MAX_QUEUES][32];
};

static int my_pci_probe(struct pci_dev *pdev,
                        const struct pci_device_id *id)
{
    struct my_pci_dev *dev;
    int ret, i;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->pdev = pdev;
    dev->num_queues = num_online_cpus();
    if (dev->num_queues > MAX_QUEUES)
        dev->num_queues = MAX_QUEUES;

    /* Enable device and map BARs... */
    ret = pcim_enable_device(pdev);
    if (ret)
        return ret;

    ret = pcim_iomap_regions(pdev, BIT(0), "my_driver");
    if (ret)
        return ret;

    dev->regs = pcim_iomap_table(pdev)[0];

    /* Prepare IRQ names */
    for (i = 0; i < dev->num_queues; i++)
        snprintf(dev->irq_names[i], 32, "my_drv-q%d", i);

    /* Set up MSI-X */
    ret = pci_alloc_irq_vectors(pdev, 1, dev->num_queues,
                                 PCI_IRQ_MSIX | PCI_IRQ_MSI);
    if (ret < 0)
        return ret;

    dev->num_irqs = ret;

    /* Initialize queues and register IRQs */
    for (i = 0; i < dev->num_irqs; i++) {
        dev->queues[i].dev = dev;
        dev->queues[i].queue_id = i;

        ret = devm_request_irq(&pdev->dev,
                                pci_irq_vector(pdev, i),
                                queue_irq_handler, 0,
                                dev->irq_names[i],
                                &dev->queues[i]);
        if (ret)
            return ret;
    }

    pci_set_drvdata(pdev, dev);

    dev_info(&pdev->dev, "Initialized with %d IRQ vectors\n", dev->num_irqs);
    return 0;
}

static void my_pci_remove(struct pci_dev *pdev)
{
    pci_free_irq_vectors(pdev);
}
```

## Summary

| Function | Purpose |
|----------|---------|
| `pci_alloc_irq_vectors()` | Allocate MSI/MSI-X vectors |
| `pci_alloc_irq_vectors_affinity()` | Allocate with CPU affinity |
| `pci_irq_vector()` | Get Linux IRQ number for vector |
| `pci_free_irq_vectors()` | Release vectors |
| `irq_set_affinity_hint()` | Suggest CPU affinity |

**Best practices:**
1. Request MSI-X first, fall back to MSI, then legacy
2. Match vector count to queue count
3. Use affinity for better cache performance
4. Use `devm_request_irq()` for automatic cleanup

## Further Reading

- [MSI HOWTO](https://docs.kernel.org/PCI/msi-howto.html) - Complete MSI/MSI-X guide
- [IRQ Affinity](https://docs.kernel.org/core-api/genericirq.html) - IRQ subsystem
