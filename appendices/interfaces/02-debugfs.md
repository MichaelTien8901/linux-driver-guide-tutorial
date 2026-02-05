---
layout: default
title: "B.2 debugfs"
parent: "Appendix B: Kernel Interfaces"
nav_order: 2
---

# debugfs

Debug-only interface - not for production use by applications.

## Creating debugfs Files

```c
#include <linux/debugfs.h>

struct my_dev {
    struct dentry *debugfs_dir;
    u32 debug_counter;
    bool debug_flag;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_dev *dev;

    /* Create directory */
    dev->debugfs_dir = debugfs_create_dir("my_driver", NULL);

    /* Simple integer file */
    debugfs_create_u32("counter", 0644, dev->debugfs_dir,
                       &dev->debug_counter);

    /* Boolean file (Y/N) */
    debugfs_create_bool("enabled", 0644, dev->debugfs_dir,
                        &dev->debug_flag);

    return 0;
}

static void my_remove(struct platform_device *pdev)
{
    struct my_dev *dev = platform_get_drvdata(pdev);

    debugfs_remove_recursive(dev->debugfs_dir);
}
```

## Custom File Operations

For complex debug output:

```c
static int stats_show(struct seq_file *s, void *unused)
{
    struct my_dev *dev = s->private;

    seq_printf(s, "TX packets: %llu\n", dev->tx_packets);
    seq_printf(s, "RX packets: %llu\n", dev->rx_packets);
    seq_printf(s, "Errors: %u\n", dev->errors);

    return 0;
}
DEFINE_SHOW_ATTRIBUTE(stats);

/* In probe */
debugfs_create_file("stats", 0444, dev->debugfs_dir, dev, &stats_fops);
```

## Debugfs Helpers

| Function | Creates |
|----------|---------|
| `debugfs_create_u8` | 8-bit unsigned |
| `debugfs_create_u32` | 32-bit unsigned |
| `debugfs_create_u64` | 64-bit unsigned |
| `debugfs_create_bool` | Boolean (Y/N) |
| `debugfs_create_x32` | Hex 32-bit |
| `debugfs_create_file` | Custom operations |
| `debugfs_create_blob` | Binary data |

## Using debugfs

```bash
# Mount (usually automatic)
mount -t debugfs none /sys/kernel/debug

# Read value
cat /sys/kernel/debug/my_driver/counter

# Write value
echo 100 > /sys/kernel/debug/my_driver/counter

# View stats
cat /sys/kernel/debug/my_driver/stats
```

{: .warning }
> debugfs may not be mounted in production. Don't rely on it for functionality - only debugging.

## Summary

- Use for debug info, register dumps, statistics
- Not ABI stable - can change without warning
- Use `debugfs_create_*` helpers for simple types
- Use `DEFINE_SHOW_ATTRIBUTE` for complex output

## Further Reading

- [debugfs Documentation](https://docs.kernel.org/filesystems/debugfs.html)
