---
layout: default
title: "1.5 QEMU Testing"
parent: "Part 1: Getting Started"
nav_order: 6
---

# Testing with QEMU

QEMU allows you to test kernel modules without physical hardware. This is invaluable for development and debugging.

## Why QEMU?

- **No hardware needed**: Test ARM64 modules on x86 host
- **Fast iteration**: Reboot virtual machine in seconds
- **Safe testing**: Crashes don't affect your system
- **Debugging**: GDB integration for kernel debugging
- **CI/CD friendly**: Automated testing in pipelines

## QEMU Overview

QEMU can emulate various architectures:

| Command | Architecture |
|---------|--------------|
| `qemu-system-aarch64` | ARM64 (AArch64) |
| `qemu-system-arm` | ARM32 |
| `qemu-system-x86_64` | x86_64 |

For embedded development, we'll focus on ARM64.

## Quick Start: Minimal Boot

Let's boot a minimal ARM64 Linux system:

### Build a Minimal Kernel

```bash
cd /kernel/linux

# Configure for ARM64 with minimal options
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig

# Enable useful options for testing
./scripts/config --enable CONFIG_MODULES
./scripts/config --enable CONFIG_MODULE_UNLOAD
./scripts/config --enable CONFIG_PRINTK
./scripts/config --enable CONFIG_DEVTMPFS
./scripts/config --enable CONFIG_DEVTMPFS_MOUNT

# Build the kernel
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image
```

### Create a Minimal Root Filesystem

Using BusyBox for a simple initramfs:

```bash
# Create directory structure
mkdir -p /tmp/initramfs/{bin,sbin,etc,proc,sys,dev,lib,lib64,tmp}
cd /tmp/initramfs

# Download and build BusyBox (static)
wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xf busybox-1.36.1.tar.bz2
cd busybox-1.36.1
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- install CONFIG_PREFIX=/tmp/initramfs
```

Create init script `/tmp/initramfs/init`:

```bash
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

echo "Welcome to Linux Driver Development Testing Environment"
echo "Load your module with: insmod /path/to/your.ko"
echo ""

exec /bin/sh
```

Make it executable:

```bash
chmod +x /tmp/initramfs/init
```

Create the initramfs:

```bash
cd /tmp/initramfs
find . | cpio -H newc -o | gzip > /tmp/initramfs.cpio.gz
```

### Boot with QEMU

```bash
qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a57 \
    -m 512M \
    -kernel /kernel/linux/arch/arm64/boot/Image \
    -initrd /tmp/initramfs.cpio.gz \
    -append "console=ttyAMA0" \
    -nographic
```

Press `Ctrl+A X` to exit QEMU.

## Testing Your Module

### Copy Module to Initramfs

```bash
# Add your module to initramfs
cp /workspace/examples/part1/hello-world/hello.ko /tmp/initramfs/

# Rebuild initramfs
cd /tmp/initramfs
find . | cpio -H newc -o | gzip > /tmp/initramfs.cpio.gz
```

### Boot and Test

```bash
qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a57 \
    -m 512M \
    -kernel /kernel/linux/arch/arm64/boot/Image \
    -initrd /tmp/initramfs.cpio.gz \
    -append "console=ttyAMA0" \
    -nographic
```

Inside QEMU:

```bash
# Load module
insmod /hello.ko
# [    1.234567] Hello, World! Module loaded.

# Check it's loaded
lsmod
# Module                  Size  Used by
# hello                  16384  0

# Unload module
rmmod hello
# [    1.345678] Goodbye, World! Module unloaded.
```

## Using a Pre-built Root Filesystem

For more complex testing, use a pre-built Debian/Ubuntu root filesystem.

### Download Debian ARM64 Rootfs

```bash
# Create disk image
qemu-img create -f qcow2 /tmp/debian-arm64.qcow2 4G

# Download Debian installer (or use debootstrap)
# See: https://www.debian.org/distrib/netinst
```

### Alternative: Use Buildroot

Buildroot creates minimal, customized root filesystems:

```bash
git clone https://github.com/buildroot/buildroot
cd buildroot
make qemu_aarch64_virt_defconfig
make -j$(nproc)
# Output: output/images/rootfs.cpio
```

## Sharing Files with QEMU

### Using 9P Filesystem (VirtFS)

Share a directory between host and QEMU:

```bash
qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a57 \
    -m 512M \
    -kernel /kernel/linux/arch/arm64/boot/Image \
    -initrd /tmp/initramfs.cpio.gz \
    -append "console=ttyAMA0" \
    -nographic \
    -fsdev local,id=shared,path=/workspace/examples,security_model=mapped \
    -device virtio-9p-pci,fsdev=shared,mount_tag=hostshare
```

Inside QEMU:

```bash
mkdir -p /mnt/host
mount -t 9p -o trans=virtio hostshare /mnt/host

# Now access your modules
insmod /mnt/host/part1/hello-world/hello.ko
```

## Debugging with GDB

### Start QEMU with GDB Server

```bash
qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a57 \
    -m 512M \
    -kernel /kernel/linux/arch/arm64/boot/Image \
    -initrd /tmp/initramfs.cpio.gz \
    -append "console=ttyAMA0 nokaslr" \
    -nographic \
    -s -S  # -s = gdb on :1234, -S = wait for gdb
```

### Connect with GDB

In another terminal:

```bash
aarch64-linux-gnu-gdb /kernel/linux/vmlinux
(gdb) target remote :1234
(gdb) continue
```

{: .tip }
The `nokaslr` boot parameter disables kernel address randomization, making debugging easier.

## QEMU Convenience Script

Create `/workspace/scripts/run-qemu.sh`:

```bash
#!/bin/bash
# Run QEMU with kernel and initramfs

KERNEL=${KERNEL:-/kernel/linux/arch/arm64/boot/Image}
INITRD=${INITRD:-/tmp/initramfs.cpio.gz}
MEMORY=${MEMORY:-512M}
SHARED=${SHARED:-/workspace/examples}

qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a57 \
    -m "$MEMORY" \
    -kernel "$KERNEL" \
    -initrd "$INITRD" \
    -append "console=ttyAMA0" \
    -nographic \
    -fsdev local,id=shared,path="$SHARED",security_model=mapped \
    -device virtio-9p-pci,fsdev=shared,mount_tag=hostshare
```

## Troubleshooting

### "Kernel panic - not syncing: No init found"

Your initramfs is missing `/init` or it's not executable:

```bash
chmod +x /tmp/initramfs/init
```

### "Unable to mount root fs"

Check initramfs path and format. Rebuild with:

```bash
find . | cpio -H newc -o | gzip > /tmp/initramfs.cpio.gz
```

### Module Version Mismatch

Ensure module was built against the same kernel you're booting:

```bash
# Check kernel version
cat /proc/version

# Check module was built against same kernel
modinfo hello.ko | grep vermagic
```

## Summary

With QEMU, you can:

1. Build kernel and modules for ARM64
2. Create minimal initramfs with your modules
3. Boot and test without hardware
4. Debug with GDB if needed

This workflow enables rapid development and testing of kernel drivers.

## Next Steps

Congratulations! You've completed Part 1. Continue to [Part 2: Kernel Fundamentals]({% link part2/index.md %}) to learn more about kernel internals.
