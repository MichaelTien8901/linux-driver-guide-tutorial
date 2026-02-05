---
layout: default
title: "Learning Paths"
nav_order: 2
---

# Learning Paths

Not sure where to start? Choose a learning path based on your experience level or role.

---

## By Experience Level

### Beginner Path
{: .text-green-200 }

**Prerequisites**: C programming basics, Linux command line familiarity

**Goal**: Write and test your first kernel module

| Step | Part | Topics | Time |
|------|------|--------|------|
| 1 | [Part 1]({% link part1/index.md %}) | Environment setup, Hello World module | 2 hours |
| 2 | [Part 2]({% link part2/index.md %}) | Kernel architecture, module lifecycle, coding style | 3 hours |
| 3 | [Part 3]({% link part3/index.md %}) | Character devices - your first real driver | 4 hours |

**You'll build**: A character device driver that can read/write data from user space.

---

### Intermediate Path
{: .text-yellow-200 }

**Prerequisites**: Completed beginner path OR have written basic modules before

**Goal**: Master concurrency, memory, and the device model

| Step | Part | Topics | Time |
|------|------|--------|------|
| 1 | [Part 4]({% link part4/index.md %}) | Spinlocks, mutexes, work queues, RCU | 4 hours |
| 2 | [Part 5]({% link part5/index.md %}) | kmalloc, DMA, memory mapping | 4 hours |
| 3 | [Part 6]({% link part6/index.md %}) | Device model, platform drivers, devm_* | 3 hours |
| 4 | [Part 7]({% link part7/index.md %}) | Interrupt handling, threaded IRQs | 3 hours |
| 5 | [Part 8]({% link part8/index.md %}) | Device tree bindings, platform bus | 3 hours |

**You'll build**: A platform driver with device tree support, proper resource management, and interrupt handling.

---

### Advanced Path
{: .text-red-200 }

**Prerequisites**: Completed intermediate path OR experienced with driver basics

**Goal**: Master subsystem-specific drivers and complex hardware

| Step | Part | Topics | Time |
|------|------|--------|------|
| 1 | [Part 9]({% link part9/index.md %}) | I2C, SPI, GPIO subsystems | 4 hours |
| 2 | [Part 10]({% link part10/index.md %}) | PWM, Watchdog, HWMON, LED | 4 hours |
| 3 | [Part 11]({% link part11/index.md %}) | IIO, RTC, Regulator, Clock | 5 hours |
| 4 | [Part 12]({% link part12/index.md %}) | Network device drivers | 4 hours |
| 5 | [Part 13]({% link part13/index.md %}) | Block device drivers | 3 hours |

**You'll build**: Production-quality I2C sensor driver, IIO data acquisition driver, network interface driver.

---

### Professional Path
{: .text-purple-200 }

**Prerequisites**: Completed advanced path OR significant driver development experience

**Goal**: Production drivers, upstream contribution, complex debugging

| Step | Part | Topics | Time |
|------|------|--------|------|
| 1 | [Part 14]({% link part14/index.md %}) | USB drivers, gadget framework | 5 hours |
| 2 | [Part 15]({% link part15/index.md %}) | PCIe drivers, MSI/MSI-X, DMA | 5 hours |
| 3 | [Part 16]({% link part16/index.md %}) | Power management, runtime PM | 3 hours |
| 4 | [Debugging]({% link appendices/debugging/index.md %}) | ftrace, kprobes, KGDB, crash | 4 hours |
| 5 | [Testing]({% link appendices/testing/index.md %}) | KUnit, kselftest, fuzzing | 3 hours |
| 6 | [Upstreaming]({% link appendices/upstreaming/index.md %}) | Patch submission, review process | 2 hours |

**You'll achieve**: Production-quality drivers, kernel upstream contributions, advanced debugging skills.

---

## By Role

### Embedded Systems Engineer

You work with SoCs, custom boards, and embedded Linux.

**Recommended path**:

```
Part 1 → Part 2 → Part 6 → Part 8 → Part 9 → Part 10 → Part 11 → Part 16
```

**Focus areas**:
- Platform drivers and device tree
- I2C/SPI sensor integration
- GPIO, PWM, and peripheral control
- Power management

---

### Hardware Engineer Transitioning to Software

You understand hardware but are new to Linux kernel programming.

**Recommended path**:

```
Part 1 → Part 2 → Part 3 → Part 6 → Part 7 → Part 8 → Part 9
```

**Focus areas**:
- Kernel/user space boundary
- Device model concepts
- Register access (ioremap)
- Interrupt handling
- Device tree bindings

---

### IoT Developer

You build connected devices with sensors and actuators.

**Recommended path**:

```
Part 1 → Part 2 → Part 3 → Part 9 → Part 10 → Part 11 → Part 14 (USB gadget)
```

**Focus areas**:
- I2C/SPI for sensors
- IIO subsystem for data acquisition
- LED and PWM for feedback
- USB gadget for connectivity

---

### Storage/Filesystem Developer

You work on storage systems and need block device knowledge.

**Recommended path**:

```
Part 1 → Part 2 → Part 4 → Part 5 → Part 13 → Part 15
```

**Focus areas**:
- Concurrency (Part 4) - critical for storage
- Memory and DMA (Part 5)
- Block device drivers (Part 13)
- PCIe for NVMe-style devices (Part 15)

---

### Network Developer

You work on network hardware or virtual network devices.

**Recommended path**:

```
Part 1 → Part 2 → Part 4 → Part 5 → Part 12 → Part 15 → Part 16
```

**Focus areas**:
- Concurrency for packet processing (Part 4)
- DMA for high-throughput (Part 5)
- Network device drivers (Part 12)
- PCIe for NICs (Part 15)
- Power management for network wakeup (Part 16)

---

### Kernel Contributor

You want to contribute drivers to the Linux kernel.

**Recommended path**:

```
Full guide + Appendix F (Upstreaming) + Appendix E (Testing)
```

**Focus areas**:
- All core concepts
- Coding style (checkpatch compliance)
- Testing (KUnit, kselftest)
- Patch submission workflow
- Documentation and bindings

---

## Quick Reference

| I want to... | Start here |
|--------------|------------|
| Learn the basics | [Part 1]({% link part1/index.md %}) |
| Write a char device | [Part 3]({% link part3/index.md %}) |
| Use device tree | [Part 8]({% link part8/index.md %}) |
| Write an I2C driver | [Part 9]({% link part9/index.md %}) |
| Handle interrupts | [Part 7]({% link part7/index.md %}) |
| Debug driver issues | [Debugging]({% link appendices/debugging/index.md %}) |
| Upstream a driver | [Upstreaming]({% link appendices/upstreaming/index.md %}) |

---

{: .tip }
**Not sure?** Start with the **Beginner Path**. The concepts build on each other, and you can always skip ahead once you're comfortable.
