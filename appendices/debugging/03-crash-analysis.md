---
layout: default
title: "A.3 Crash Analysis"
parent: "Appendix A: Debugging"
nav_order: 3
---

# Crash Analysis

Analyzing kernel crashes with kdump and crash tool.

## Kdump Setup

Kdump captures memory when the kernel crashes:

```bash
# Install (Ubuntu/Debian)
sudo apt install kdump-tools crash

# Install (Fedora/RHEL)
sudo dnf install kexec-tools crash

# Enable
sudo systemctl enable kdump

# Verify
cat /sys/kernel/kexec_crash_loaded  # Should be 1
```

After a crash, dumps are saved to `/var/crash/`.

## Reading Oops Messages

When your driver crashes, the kernel prints an "Oops":

```
BUG: unable to handle page fault for address: ffff8801234567
Oops: 0000 [#1] SMP
CPU: 0 PID: 1234 Comm: my_app Tainted: G    O 6.6.0
RIP: 0010:my_driver_read+0x42/0x100 [my_driver]
...
Call Trace:
 vfs_read+0x9d/0x150
 ksys_read+0x5f/0xe0
 do_syscall_64+0x5c/0x90
```

**Key information:**
- `RIP`: Where crash occurred (`my_driver_read+0x42`)
- `Call Trace`: How we got there
- `Tainted: G O`: Module loaded (O = out-of-tree)

## Finding the Line Number

```bash
# Disassemble to find offset
objdump -dS my_driver.ko | less

# Or use addr2line
addr2line -e my_driver.ko 0x42
```

## Using crash Tool

```bash
# Open crash dump
sudo crash /usr/lib/debug/lib/modules/$(uname -r)/vmlinux \
           /var/crash/*/vmcore

# Basic commands in crash:
crash> bt           # Backtrace
crash> log          # Kernel log
crash> ps           # Process list
crash> mod          # Loaded modules
crash> dis my_func  # Disassemble function
crash> struct device ffff8801234567  # View structure
```

## Common Crash Types

| Oops Code | Meaning |
|-----------|---------|
| `0000` | Read from bad address |
| `0002` | Write to bad address |
| `0010` | User-mode access |
| `0011` | Kernel-mode write |

## Debugging Tips

1. **NULL pointer**: Check all `devm_` allocations
2. **Use after free**: Verify object lifetime
3. **Stack overflow**: Reduce local variables, use heap
4. **Deadlock**: Check lock ordering (use lockdep)

## Enabling Debug Info

For better crash analysis, build with debug info:

```makefile
# In your Makefile
ccflags-y += -g
```

Or kernel config:
```
CONFIG_DEBUG_INFO=y
CONFIG_DEBUG_INFO_DWARF4=y
```

## Summary

| Tool | Use For |
|------|---------|
| Oops message | Quick crash location |
| `addr2line` | Line number from offset |
| `crash` | Post-mortem analysis |
| `objdump -dS` | Source + assembly |

## Further Reading

- [Kernel Oops](https://docs.kernel.org/admin-guide/bug-hunting.html)
- [Kdump](https://docs.kernel.org/admin-guide/kdump/kdump.html)
- [Crash Tool](https://crash-utility.github.io/)
