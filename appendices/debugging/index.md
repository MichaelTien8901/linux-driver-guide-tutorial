---
layout: default
title: "Appendix A: Debugging"
nav_order: 17
has_children: true
---

# Appendix A: Debugging

Practical debugging techniques for kernel driver development.

## Debugging Strategy

```mermaid
flowchart TB
    BUG["Bug/Issue"]
    BUG --> Q1{"Can reproduce?"}
    Q1 -->|Yes| LOG["Add printk/dev_*"]
    Q1 -->|No| TRACE["Enable tracing"]
    LOG --> ANALYZE["Analyze output"]
    TRACE --> ANALYZE
    ANALYZE --> Q2{"Found cause?"}
    Q2 -->|Yes| FIX["Fix"]
    Q2 -->|No| DEBUG["debugfs/kgdb"]
    DEBUG --> FIX

    style LOG fill:#7a8f73,stroke:#2e7d32
    style TRACE fill:#738f99,stroke:#0277bd
```

## Tools Overview

| Tool | Use For | Overhead |
|------|---------|----------|
| `printk`/`dev_*` | Quick debugging, always available | Low |
| Dynamic debug | Enable/disable at runtime | Very low |
| ftrace | Function flow, latency | Medium |
| kprobes | Inspect without recompile | Medium |
| KGDB | Interactive debugging | High |
| kdump/crash | Post-mortem analysis | None (captures on crash) |

## Chapters

| Chapter | What You'll Learn |
|---------|-------------------|
| [Logging]({% link appendices/debugging/01-logging.md %}) | printk, dev_*, dynamic debug |
| [Tracing]({% link appendices/debugging/02-tracing.md %}) | ftrace, kprobes |
| [Crash Analysis]({% link appendices/debugging/03-crash-analysis.md %}) | kdump and crash tool |

## Quick Reference

```bash
# View kernel log
dmesg -w

# Filter by driver
dmesg | grep my_driver

# Enable dynamic debug
echo 'module my_driver +p' > /sys/kernel/debug/dynamic_debug/control

# View function trace
echo function > /sys/kernel/debug/tracing/current_tracer
cat /sys/kernel/debug/tracing/trace
```

## Further Reading

- [Kernel Debugging](https://docs.kernel.org/dev-tools/index.html) - Official docs
- [Dynamic Debug](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)
- [ftrace](https://docs.kernel.org/trace/ftrace.html)
