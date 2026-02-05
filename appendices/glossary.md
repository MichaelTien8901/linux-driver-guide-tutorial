---
layout: default
title: "Appendix G: Glossary"
nav_order: 23
---

# Kernel Terminology Glossary

Quick reference for terms used throughout this guide.

## A

**Atomic context**
: Code path where sleeping is not allowed (interrupt handlers, spinlock-held regions). Use `GFP_ATOMIC` for allocations.

**Autosuspend**
: Runtime PM feature that delays suspend after the last `pm_runtime_put()`, avoiding rapid suspend/resume cycles.

## B

**BAR (Base Address Register)**
: PCI configuration register that defines a memory or I/O region the device exposes to the system.

**bio**
: Block I/O structure representing a segment of contiguous sectors. Multiple bios may be merged into a request.

**Bottom half**
: Deferred work from an interrupt handler (tasklets, workqueues, threaded IRQs). Runs with interrupts enabled.

**Bus mastering**
: Ability of a PCI device to initiate DMA transfers. Enabled via `pci_set_master()`.

## C

**Carrier**
: Network link status. `netif_carrier_on()` indicates link is up; `netif_carrier_off()` indicates link is down.

**cdev**
: Character device structure. Registered with `cdev_add()` to associate a device number with file operations.

**Coherent DMA**
: DMA memory that is automatically synchronized between CPU and device. Allocated with `dma_alloc_coherent()`.

**Completion**
: Synchronization primitive for waiting until another thread signals an event. See `wait_for_completion()`.

**container_of**
: Macro to get the containing structure from a pointer to one of its members.

## D

**Deferred probe**
: When `probe()` returns `-EPROBE_DEFER`, the driver will be retried later after other drivers load.

**Device Tree (DT)**
: Data structure describing hardware topology. Parsed at boot to create platform devices.

**devres (Device-managed resources)**
: Resources allocated with `devm_*` functions that are automatically freed when the device is removed.

**DMA (Direct Memory Access)**
: Hardware mechanism allowing devices to transfer data to/from memory without CPU involvement.

## E

**Endpoint**
: USB communication channel. Each endpoint has a direction (IN/OUT) and type (bulk, interrupt, control, isochronous).

**EXPORT_SYMBOL**
: Macro that makes a kernel symbol available to loadable modules.

## F

**file_operations (fops)**
: Structure containing function pointers for file operations (read, write, ioctl, etc.) on a character device.

**Fixed-link**
: Network configuration without a PHY chip, used for MAC-to-MAC or MAC-to-switch connections.

## G

**gendisk**
: Structure representing a block device (disk). Contains capacity, name, and link to request queue.

**GFP flags**
: Get Free Pages flags controlling memory allocation behavior (e.g., `GFP_KERNEL`, `GFP_ATOMIC`).

**gpio_chip**
: Structure for GPIO controller drivers, defining operations for requesting, setting, and reading GPIOs.

## H

**HZ**
: Kernel timer frequency (jiffies per second). Typically 100, 250, or 1000.

**HWMON**
: Hardware monitoring subsystem for temperature, voltage, and fan sensors.

## I

**IIO (Industrial I/O)**
: Subsystem for ADCs, DACs, accelerometers, and other data acquisition devices.

**Interface (USB)**
: Logical function within a USB device. Drivers bind to interfaces, not devices.

**ioremap**
: Function to map physical device memory into kernel virtual address space.

**IRQ**
: Interrupt Request. Hardware signal that interrupts CPU to handle an event.

## J

**Jiffies**
: Kernel time counter incremented HZ times per second.

## K

**kmap_local_page**
: Temporarily maps a page to kernel address space. Use for highmem pages.

**kmalloc**
: Kernel memory allocator for small, physically contiguous allocations.

**kobject**
: Base object in the kernel device model. Provides reference counting and sysfs representation.

**kref**
: Kernel reference counter. Calls a release function when count reaches zero.

## L

**Lockdep**
: Kernel lock validator that detects potential deadlocks at runtime.

## M

**Major/minor number**
: Device number pair. Major identifies the driver; minor identifies specific device.

**MMIO (Memory-Mapped I/O)**
: Hardware registers accessed through memory read/write operations.

**MSI/MSI-X (Message Signaled Interrupts)**
: Modern interrupt mechanism for PCI devices, eliminating shared IRQ issues.

**Mutex**
: Sleeping lock for protecting critical sections. Cannot be held in atomic context.

## N

**NAPI (New API)**
: Network polling mechanism that switches from interrupts to polling under high load.

**net_device**
: Structure representing a network interface (eth0, wlan0, etc.).

**net_device_ops**
: Operations structure for network drivers (open, stop, start_xmit, etc.).

## O

**of_* functions**
: Device Tree parsing functions (of_property_read_u32, of_get_named_gpio, etc.).

## P

**PHY**
: Physical layer transceiver chip in Ethernet networks. Managed via PHYLIB.

**Platform device**
: Non-discoverable device described in Device Tree or board files.

**Probe**
: Driver callback called when a matching device is found. Initializes the device.

## R

**RCU (Read-Copy-Update)**
: Synchronization mechanism optimized for read-heavy workloads.

**Regmap**
: API for accessing device registers over I2C, SPI, or memory-mapped I/O.

**Request (block)**
: Block layer structure representing an I/O operation. Contains one or more bios.

**Runtime PM**
: Power management for individual devices while the system is running.

## S

**Scatter-gather**
: DMA technique using multiple non-contiguous memory segments.

**Sector**
: Basic unit of block device addressing (historically 512 bytes).

**sk_buff (skb)**
: Socket buffer structure representing a network packet.

**Spinlock**
: Busy-wait lock for short critical sections. Can be held in interrupt context.

**Streaming DMA**
: DMA mapping that requires explicit sync operations. More efficient than coherent for one-time transfers.

**Sysfs**
: Virtual filesystem exposing kernel objects and device attributes.

## T

**Tag set**
: blk-mq structure defining hardware queue configuration.

**Tasklet**
: Soft IRQ mechanism for deferring work from interrupt handlers.

**Top half**
: The interrupt handler itself. Runs with interrupts disabled, should be fast.

## U

**URB (USB Request Block)**
: Structure for USB data transfers. Submitted asynchronously, completed via callback.

**udev**
: User-space device manager that creates device nodes and handles hotplug events.

## V

**VID/PID**
: Vendor ID and Product ID. Used to identify USB and PCI devices.

**vmalloc**
: Kernel memory allocator for large, virtually contiguous (but physically scattered) allocations.

## W

**Wakeup source**
: Device capable of waking the system from suspend.

**Work queue**
: Mechanism for deferring work to a kernel thread context.

## Further Reading

- [Kernel Glossary](https://docs.kernel.org/glossary.html) - Official kernel glossary
- [Linux Kernel Documentation](https://docs.kernel.org/) - Comprehensive documentation
