---
layout: default
title: "Appendix H: References"
nav_order: 24
---

# Curated References

Essential resources for Linux driver development, organized by topic.

## Official Kernel Documentation

### Core References
- [Kernel Documentation](https://docs.kernel.org/) - Primary documentation source
- [Driver API](https://docs.kernel.org/driver-api/index.html) - Driver-specific APIs
- [Core API](https://docs.kernel.org/core-api/index.html) - Fundamental kernel APIs

### By Subsystem

| Subsystem | Documentation |
|-----------|---------------|
| Device Model | [Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html) |
| Device Tree | [Devicetree](https://docs.kernel.org/devicetree/index.html) |
| Block Layer | [Block](https://docs.kernel.org/block/index.html) |
| Network | [Networking](https://docs.kernel.org/networking/index.html) |
| USB | [USB API](https://docs.kernel.org/driver-api/usb/index.html) |
| PCI | [PCI](https://docs.kernel.org/PCI/index.html) |
| I2C | [I2C](https://docs.kernel.org/i2c/index.html) |
| SPI | [SPI](https://docs.kernel.org/spi/index.html) |
| GPIO | [GPIO](https://docs.kernel.org/driver-api/gpio/index.html) |
| IIO | [Industrial I/O](https://docs.kernel.org/driver-api/iio/index.html) |
| RTC | [RTC](https://docs.kernel.org/driver-api/rtc.html) |
| Regulator | [Regulator](https://docs.kernel.org/power/regulator/index.html) |
| Clock | [Clock Framework](https://docs.kernel.org/driver-api/clk.html) |
| PWM | [PWM](https://docs.kernel.org/driver-api/pwm.html) |
| Watchdog | [Watchdog](https://docs.kernel.org/watchdog/index.html) |
| HWMON | [Hardware Monitoring](https://docs.kernel.org/hwmon/index.html) |
| LED | [LED](https://docs.kernel.org/leds/index.html) |
| Power Management | [Power](https://docs.kernel.org/power/index.html) |

### Memory and DMA
- [Memory Allocation](https://docs.kernel.org/core-api/memory-allocation.html)
- [DMA API Guide](https://docs.kernel.org/core-api/dma-api.html)
- [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)

### Synchronization
- [Locking](https://docs.kernel.org/locking/index.html)
- [RCU](https://docs.kernel.org/RCU/index.html)
- [Atomic Operations](https://docs.kernel.org/core-api/atomic_ops.html)

### Debugging and Testing
- [Debugging](https://docs.kernel.org/dev-tools/index.html)
- [KUnit](https://docs.kernel.org/dev-tools/kunit/index.html)
- [KASAN](https://docs.kernel.org/dev-tools/kasan.html)
- [Ftrace](https://docs.kernel.org/trace/ftrace.html)

### Development Process
- [Submitting Patches](https://docs.kernel.org/process/submitting-patches.html)
- [Coding Style](https://docs.kernel.org/process/coding-style.html)
- [MAINTAINERS](https://docs.kernel.org/process/maintainers.html)

## Books

### Recommended

| Title | Notes |
|-------|-------|
| **Linux Device Drivers, 3rd Edition** | Classic reference (covers 2.6, concepts still relevant). [Free online](https://lwn.net/Kernel/LDD3/) |
| **Linux Kernel Development** (Robert Love) | Excellent kernel internals overview |
| **Essential Linux Device Drivers** (Sreekrishnan Venkateswaran) | Good subsystem coverage |

### Advanced Topics

| Title | Notes |
|-------|-------|
| **Understanding the Linux Kernel** (Bovet & Cesati) | Deep kernel internals |
| **Professional Linux Kernel Architecture** (Mauerer) | Comprehensive reference |
| **Linux Kernel Networking** (Rami Rosen) | Network stack deep dive |

## Online Resources

### Source Code Browsing
- [Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source) - Searchable kernel source
- [LXR (Linux Cross Reference)](https://lxr.sourceforge.io/) - Alternative source browser
- [GitHub Mirror](https://github.com/torvalds/linux) - Linux kernel on GitHub

### Mailing Lists and News
- [LWN.net](https://lwn.net/) - Linux Weekly News (essential reading)
- [LKML (Linux Kernel Mailing List)](https://lkml.org/) - Main development list
- [Kernel Newbies](https://kernelnewbies.org/) - Resources for beginners

### Tutorials and Guides
- [Kernel Newbies - First Kernel Patch](https://kernelnewbies.org/FirstKernelPatch)
- [Bootlin Training Materials](https://bootlin.com/docs/) - Free embedded Linux training
- [The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)

## Tools

### Development
| Tool | Purpose |
|------|---------|
| `checkpatch.pl` | Code style checker |
| `sparse` | Static analysis |
| `coccinelle` | Semantic patching |
| `scripts/get_maintainer.pl` | Find patch reviewers |

### Testing
| Tool | Purpose |
|------|---------|
| QEMU | Hardware emulation |
| KUnit | Unit testing |
| kselftest | Integration tests |
| syzkaller | Fuzzing |

### Debugging
| Tool | Purpose |
|------|---------|
| `dmesg` | Kernel log viewer |
| `ftrace` | Function/event tracing |
| `perf` | Performance analysis |
| `crash` | Crash dump analysis |

## Specifications

### Hardware Protocols
- [USB Specification](https://www.usb.org/documents) - USB-IF documents
- [PCI Express Specification](https://pcisig.com/specifications) - PCI-SIG (requires membership)
- [I2C Specification](https://www.nxp.com/docs/en/user-guide/UM10204.pdf) - NXP I2C-bus specification
- [SPI Overview](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface) - Wikipedia (no formal spec)

### Device Tree
- [Devicetree Specification](https://www.devicetree.org/specifications/) - Official specification
- [Kernel DT Bindings](https://docs.kernel.org/devicetree/bindings/index.html) - Binding documentation

## Kernel Versions

This guide targets **Linux 6.6 LTS**. Check kernel changelogs for API changes:
- [KernelNewbies Changelog](https://kernelnewbies.org/LinuxChanges) - Human-readable changelogs
- [Git Changelog](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/log/) - Official commits

## Getting Help

1. **Search first**: Use Elixir to find existing drivers similar to yours
2. **Read the docs**: Check docs.kernel.org for your subsystem
3. **Ask on IRC**: `#kernelnewbies` on OFTC
4. **Mailing lists**: Use `get_maintainer.pl` to find the right list
5. **Stack Exchange**: [Unix & Linux Stack Exchange](https://unix.stackexchange.com/) for general questions
