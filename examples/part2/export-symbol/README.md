# Symbol Export Example

Demonstrates exporting and using symbols between kernel modules.

## Files

- `mylib.c` - Library module that exports symbols
- `myuser.c` - Consumer module that uses the exported symbols
- `Makefile` - Builds both modules

## Exported Symbols (mylib)

| Symbol | Type | Export | Description |
|--------|------|--------|-------------|
| `mylib_increment` | function | EXPORT_SYMBOL | Increment counter |
| `mylib_get_count` | function | EXPORT_SYMBOL | Get counter value |
| `mylib_multiply` | function | EXPORT_SYMBOL | Multiply two numbers |
| `mylib_reset` | function | EXPORT_SYMBOL_GPL | Reset counter (GPL only) |

## Building

```bash
make KERNEL_DIR=/kernel/linux ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

## Testing

### Load Modules (Order Matters!)

```bash
# Load library first
sudo insmod mylib.ko
dmesg | tail -3

# Then load consumer
sudo insmod myuser.ko iterations=3
dmesg | tail -10
```

### Check Dependencies

```bash
# Show module dependencies
lsmod | grep my

# myuser depends on mylib
cat /sys/module/myuser/holders/
```

### View Exported Symbols

```bash
# Show symbols exported by mylib
cat /proc/kallsyms | grep mylib
```

### Unload Modules (Reverse Order!)

```bash
# Consumer first
sudo rmmod myuser
dmesg | tail -3

# Then library
sudo rmmod mylib
dmesg | tail -2
```

### Load Order Error

```bash
# This will fail - mylib not loaded
sudo rmmod mylib
sudo rmmod myuser  # if loaded
sudo insmod myuser.ko
# insmod: ERROR: could not insert module myuser.ko: Unknown symbol in module
```

## Key Concepts Demonstrated

1. **EXPORT_SYMBOL()** - Export symbol to all modules
2. **EXPORT_SYMBOL_GPL()** - Export symbol only to GPL modules
3. **Module dependencies** - Load order requirements
4. **External declarations** - Using `extern` for cross-module symbols
5. **MODULE_SOFTDEP()** - Declaring soft dependencies
