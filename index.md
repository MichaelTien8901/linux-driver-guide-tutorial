---
layout: default
title: Home
nav_order: 1
description: "Linux Driver Development Guide - Learn kernel driver development from basics to advanced subsystems"
permalink: /
---

# Linux Driver Development Guide

Welcome to the comprehensive Linux kernel driver development guide. This tutorial takes you from writing your first kernel module to implementing production-quality drivers for various hardware subsystems.

## What You'll Learn

- **Getting Started**: Set up a Docker-based development environment with cross-compilation and QEMU testing
- **Kernel Fundamentals**: Understand kernel space, module lifecycle, coding conventions, and error handling
- **Character Devices**: Build character device drivers with proper file operations and user-space interaction
- **Concurrency**: Master spinlocks, mutexes, RCU, and other synchronization primitives
- **Memory Management**: Learn kmalloc, DMA mappings, and memory-mapped I/O
- **Device Model**: Understand buses, drivers, device tree, and the probe/remove lifecycle
- **Subsystem Drivers**: Implement I2C, SPI, GPIO, PWM, IIO, and many more subsystem drivers
- **Advanced Topics**: Network drivers, block devices, USB, PCIe, and power management

## Quick Start

1. [Set up Docker Environment](#docker-quick-start)
2. Write your first kernel module
3. Test with QEMU - no physical hardware needed!

## How This Guide is Organized

| Section | Description | Level |
|---------|-------------|-------|
| Part 1: Getting Started | Environment setup, Docker, QEMU, first module | Beginner |
| Part 2: Core Concepts | Kernel fundamentals, coding style, data types | Beginner |
| Part 3: Character Devices | file_operations, cdev, ioctl, user-space transfer | Beginner |
| Part 4: Concurrency | Spinlocks, mutexes, atomic ops, RCU, work queues | Intermediate |
| Part 5: Memory | kmalloc, vmalloc, DMA, ioremap, memory pools | Intermediate |
| Part 6: Device Model | Buses, platform drivers, device tree, devm_* | Intermediate |
| Part 7: Interrupts | IRQ handling, threaded IRQs, tasklets, bottom halves | Intermediate |
| Part 8: Platform & DT | Device tree bindings, overlays, dt-schema | Intermediate |
| Part 9: Subsystem Drivers | I2C, SPI, GPIO, PWM, watchdog, HWMON, LED, IIO, RTC | Advanced |
| Part 10: Frameworks | Regulator, clock, network, block drivers | Advanced |
| Part 11: Bus Drivers | USB, PCIe, DMA for high-speed peripherals | Advanced |
| Part 12: Power Management | Runtime PM, suspend/resume, PM domains | Advanced |
| Part 13: Debug & Trace | printk, ftrace, kprobes, KGDB, crash analysis | Advanced |
| Part 14: Interfaces | sysfs, procfs, configfs, debugfs | Advanced |
| Part 15: Quality | KUnit, kselftest, sanitizers, fuzzing | Professional |
| Part 16: Upstreaming | Patch submission, coding style, review process | Professional |

## Target Kernel Version

This guide targets **Linux kernel 6.6 LTS** with notes for 6.1 LTS and 6.12 LTS differences where relevant.

## Prerequisites

- C programming proficiency
- Basic Linux command line skills
- Understanding of computer architecture (helpful, not required)
- No physical hardware needed - we use QEMU!

## Docker Quick Start

Don't pollute your host system - use our Docker environment:

```bash
cd docs
docker-compose up -d
docker-compose exec kernel-dev bash

# Inside container - compile a module for ARM64
cd /workspace/examples/01-hello-world
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

## Sample Code

All examples in this guide are tested and available in the `examples/` directory. Each example includes:

- Complete, buildable kernel module
- Makefile for out-of-tree compilation
- Device tree overlay (where applicable)
- QEMU test script
- Detailed README

## Contributing

Found an error? Want to improve content? Open an issue on GitHub.

---

{: .note }
This guide is community-maintained and not affiliated with the Linux Foundation. For official documentation, visit [docs.kernel.org](https://docs.kernel.org/).
