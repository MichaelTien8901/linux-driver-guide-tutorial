# Hello World Kernel Module

A minimal "Hello World" Linux kernel module demonstrating the basic structure.

## Files

- `hello.c` - Module source code
- `Makefile` - Build configuration

## Building

### For Host Kernel

```bash
make
```

### Cross-compile for ARM64

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KERNEL_DIR=/kernel/linux
```

## Testing

### Load the Module

```bash
sudo insmod hello.ko
```

### Check Kernel Log

```bash
dmesg | tail -5
```

Expected output:
```
[12345.678901] hello: Hello, World! Module loaded.
[12345.678902] hello: This is running in kernel space!
[12345.678903] hello: Current process: insmod (pid 1234)
```

### List Loaded Modules

```bash
lsmod | grep hello
```

### View Module Info

```bash
modinfo hello.ko
```

### Unload the Module

```bash
sudo rmmod hello
dmesg | tail -1
```

Expected output:
```
[12345.789012] hello: Goodbye, World! Module unloaded.
```

## What This Demonstrates

1. **Module structure**: init and exit functions
2. **Module registration**: `module_init()` and `module_exit()` macros
3. **Kernel logging**: Using `pr_info()` for output
4. **Module metadata**: LICENSE, AUTHOR, DESCRIPTION, VERSION
5. **SPDX license identifier**: Required for upstream kernel code

## Next Steps

- Add module parameters (see Part 2)
- Create a character device (see Part 3)
