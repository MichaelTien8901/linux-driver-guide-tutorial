---
layout: default
title: "A.1 Logging"
parent: "Appendix A: Debugging"
nav_order: 1
---

# Logging

The simplest and most common debugging technique.

## printk and Log Levels

```c
/* Generic printk with level */
printk(KERN_ERR "Error: %d\n", err);
printk(KERN_INFO "Initialized\n");
printk(KERN_DEBUG "Debug info\n");

/* Shorthand macros (preferred) */
pr_err("Error: %d\n", err);
pr_info("Initialized\n");
pr_debug("Debug info\n");  /* Compiled out unless DEBUG defined */
```

| Level | Macro | Use For |
|-------|-------|---------|
| KERN_EMERG | `pr_emerg` | System unusable |
| KERN_ERR | `pr_err` | Error conditions |
| KERN_WARNING | `pr_warn` | Warning |
| KERN_NOTICE | `pr_notice` | Normal but significant |
| KERN_INFO | `pr_info` | Informational |
| KERN_DEBUG | `pr_debug` | Debug messages |

## Device-Specific Logging

**Always prefer `dev_*` for drivers** - they include device name:

```c
dev_err(&pdev->dev, "Failed to init: %d\n", ret);
dev_info(&pdev->dev, "Probe successful\n");
dev_dbg(&pdev->dev, "Register value: 0x%x\n", val);

/* With error return */
return dev_err_probe(&pdev->dev, ret, "Clock failed\n");
```

Output includes device path:
```
my_driver 0000:00:1f.6: Failed to init: -22
```

## Dynamic Debug

Enable/disable debug messages at runtime without recompiling:

```bash
# Enable all debug in a module
echo 'module my_driver +p' > /sys/kernel/debug/dynamic_debug/control

# Enable specific file
echo 'file my_driver.c +p' > /sys/kernel/debug/dynamic_debug/control

# Enable specific function
echo 'func my_probe +p' > /sys/kernel/debug/dynamic_debug/control

# Add function name and line number
echo 'module my_driver +fpl' > /sys/kernel/debug/dynamic_debug/control

# View current settings
cat /sys/kernel/debug/dynamic_debug/control | grep my_driver

# Disable
echo 'module my_driver -p' > /sys/kernel/debug/dynamic_debug/control
```

{: .tip }
> Dynamic debug only works with `dev_dbg()` and `pr_debug()`. Build with `CONFIG_DYNAMIC_DEBUG=y`.

## Format Specifiers

Kernel has special format specifiers:

```c
/* Pointers */
pr_info("Device: %pOF\n", np);       /* Device tree path */
pr_info("Address: %pR\n", &res);     /* Resource (BAR) */
pr_info("MAC: %pM\n", mac_addr);     /* MAC address */
pr_info("IPv4: %pI4\n", &ip_addr);   /* IP address */

/* Error pointers */
pr_info("Error: %pe\n", ERR_PTR(-EINVAL));  /* "-EINVAL" */
```

## Viewing Logs

```bash
# Continuous watching
dmesg -w

# With human-readable timestamps
dmesg -T

# Filter by level
dmesg --level=err,warn

# journalctl (systemd)
journalctl -k -f
```

## Summary

| Function | When to Use |
|----------|-------------|
| `pr_err` | Error conditions |
| `dev_err` | Driver errors (preferred) |
| `dev_dbg` | Debug info (use with dynamic debug) |
| `dev_err_probe` | Probe failures with deferred handling |

## Further Reading

- [printk Documentation](https://docs.kernel.org/core-api/printk-basics.html)
- [Dynamic Debug](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)
