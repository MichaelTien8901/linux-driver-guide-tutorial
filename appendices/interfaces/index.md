---
layout: default
title: "Appendix B: Kernel Interfaces"
nav_order: 18
has_children: true
---

# Appendix B: Kernel Interfaces

Choosing the right interface for exposing driver functionality.

## Decision Guide

```mermaid
flowchart TB
    Q["Need to expose something?"]
    Q --> Q1{"For users or debugging?"}
    Q1 -->|Users| Q2{"Device config or generic?"}
    Q1 -->|Debugging| DEBUGFS["debugfs"]
    Q2 -->|Device config| SYSFS["sysfs<br/>(DEVICE_ATTR)"]
    Q2 -->|System-wide| PROCFS["procfs<br/>(if legacy)"]
    Q2 -->|User-configurable| CONFIGFS["configfs"]

    style SYSFS fill:#7a8f73,stroke:#2e7d32
    style DEBUGFS fill:#738f99,stroke:#0277bd
```

## Quick Comparison

| Interface | Use For | Stable ABI? | Example |
|-----------|---------|-------------|---------|
| **sysfs** | Device attributes | Yes | `/sys/class/leds/*/brightness` |
| **debugfs** | Debug info, testing | No | `/sys/kernel/debug/my_driver/` |
| **procfs** | System/process info | Yes (legacy) | `/proc/cpuinfo` |
| **configfs** | User-defined objects | Yes | `/sys/kernel/config/usb_gadget/` |

## Recommendations

| Scenario | Use |
|----------|-----|
| Device-specific attribute | sysfs (`DEVICE_ATTR`) |
| Hardware register dump | debugfs |
| Debug statistics | debugfs |
| New system info | sysfs (avoid procfs for new code) |
| User creates objects | configfs |

## Chapters

| Chapter | What You'll Learn |
|---------|-------------------|
| [sysfs]({% link appendices/interfaces/01-sysfs.md %}) | DEVICE_ATTR, attribute groups |
| [debugfs]({% link appendices/interfaces/02-debugfs.md %}) | Debug interfaces |

## Example

- **[sysfs Demo](../examples/appendices/sysfs-demo/)** - Driver with custom attributes

## Further Reading

- [sysfs](https://docs.kernel.org/filesystems/sysfs.html)
- [debugfs](https://docs.kernel.org/filesystems/debugfs.html)
- [configfs](https://docs.kernel.org/filesystems/configfs.html)
