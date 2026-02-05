# PCI Driver Skeleton

A minimal PCI driver template demonstrating core PCI driver patterns.

## What This Example Shows

- PCI device matching by VID:PID
- Managed resource allocation (`pcim_*`, `devm_*`)
- BAR mapping with `pcim_iomap_regions()`
- MSI interrupt setup
- Proper error handling and cleanup

## Adapting for Your Device

1. Find your device's VID:PID:
   ```bash
   lspci -nn
   # Output: 00:1f.6 Ethernet [0200]: Intel [8086:15d8]
   ```

2. Modify the defines in `pci_skeleton.c`:
   ```c
   #define SKELETON_VID 0x8086
   #define SKELETON_PID 0x15d8
   ```

3. Update register offsets for your device's register layout.

4. Rebuild and test.

## Building

```bash
make
```

## Testing

```bash
# Find PCI devices
lspci -nn

# Load module (will only attach if matching device exists)
sudo insmod pci_skeleton.ko

# Check kernel log
dmesg | tail -20

# If device found, you'll see:
#   pci_skeleton: Probing PCI device xxxx:xxxx
#   pci_skeleton: BAR0 mapped at ffffa000
#   pci_skeleton: IRQ 32 allocated
#   pci_skeleton: PCI skeleton driver loaded

# Unload
sudo rmmod pci_skeleton
```

## Key Code Sections

### Device Matching

```c
static const struct pci_device_id skeleton_ids[] = {
    { PCI_DEVICE(SKELETON_VID, SKELETON_PID) },
    { }
};
```

### Probe Sequence

```c
static int skeleton_probe(struct pci_dev *pdev, ...)
{
    /* 1. Enable device */
    pcim_enable_device(pdev);

    /* 2. Map BAR0 */
    pcim_iomap_regions(pdev, BIT(0), DRIVER_NAME);
    dev->regs = pcim_iomap_table(pdev)[0];

    /* 3. Enable bus mastering for DMA */
    pci_set_master(pdev);

    /* 4. Set up MSI interrupt */
    pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI | PCI_IRQ_LEGACY);
    devm_request_irq(...);

    /* 5. Initialize hardware */
    skeleton_hw_init(dev);
}
```

### Register Access

```c
static inline u32 skel_read(struct skeleton_dev *dev, int reg)
{
    return readl(dev->regs + reg);
}

static inline void skel_write(struct skeleton_dev *dev, int reg, u32 val)
{
    writel(val, dev->regs + reg);
}
```

## Files

- `pci_skeleton.c` - Complete driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- [Part 15.1: PCI Concepts](../../../part15/01-concepts.md)
- [Part 15.2: Driver Skeleton](../../../part15/02-driver-skeleton.md)

## Further Reading

- [PCI Documentation](https://docs.kernel.org/PCI/index.html)
- [PCI Driver API](https://docs.kernel.org/PCI/pci.html)
