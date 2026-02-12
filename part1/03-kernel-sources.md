---
layout: default
title: "1.3 Kernel Sources"
parent: "Part 1: Getting Started"
nav_order: 4
---

# Obtaining and Configuring Kernel Sources

To build kernel modules, you need kernel headers that match your target kernel version. This chapter covers obtaining kernel sources and preparing them for driver development.

## Understanding Kernel Versions

Linux kernel versions follow the format `major.minor.patch`:

- **Major**: Rarely changes (we're on 6.x)
- **Minor**: New features, released every ~9 weeks
- **Patch**: Bug fixes and security updates

### LTS vs Mainline

| Type | Example | Support | Use Case |
|------|---------|---------|----------|
| **LTS** | 6.6.70 | ~6 years | Production drivers |
| **Mainline** | 6.12.7 | ~3 months | Bleeding edge |

{: .tip }
This guide targets **6.6 LTS** for stability. We'll note differences for 6.1 LTS and 6.12.

## Downloading Kernel Sources

### Using Our Script

Inside the development container:

```bash
cd /workspace/scripts
./download-kernel.sh
```

This downloads kernel 6.6 LTS to `/kernel/linux-6.6.x/`.

For a different version:

```bash
./download-kernel.sh 6.1   # Download 6.1 LTS
./download-kernel.sh 6.12  # Download 6.12 LTS
```

### Manual Download

```bash
KERNEL_VERSION="6.6.70"
cd /kernel
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${KERNEL_VERSION}.tar.xz
tar xf linux-${KERNEL_VERSION}.tar.xz
ln -s linux-${KERNEL_VERSION} linux
```

## Kernel Source Structure

```
linux/
├── arch/           # Architecture-specific code
│   ├── arm64/
│   ├── x86/
│   └── ...
├── drivers/        # Device drivers (we'll study this!)
│   ├── gpio/
│   ├── i2c/
│   ├── spi/
│   └── ...
├── include/        # Header files
│   └── linux/      # Public kernel API
├── kernel/         # Core kernel code
├── Documentation/  # Kernel documentation
├── scripts/        # Build scripts
├── Kconfig         # Configuration system
└── Makefile        # Top-level Makefile
```

## Configuring the Kernel

Before building modules, configure the kernel for your target architecture.

### For ARM64

```bash
cd /kernel/linux
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
```

### For x86_64

```bash
cd /kernel/linux
make defconfig
```

### Custom Configuration

For a customized config:

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
```

This opens an interactive configuration menu.

## Preparing Headers for Module Building

After configuration, prepare the kernel for external module building:

```bash
cd /kernel/linux
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules_prepare
```

This creates:

- Version information files
- Generated headers
- Module symbol information

{: .note }
You don't need to build the entire kernel - just `modules_prepare` is sufficient for external modules.

## Verifying the Setup

Check that headers are ready:

```bash
ls /kernel/linux/include/generated/autoconf.h
# Should exist

ls /kernel/linux/Module.symvers
# Should exist (may be empty for fresh config)
```

## Environment Variables

Set these for convenience:

```bash
export KERNEL_DIR=/kernel/linux
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
```

Add to `~/.bashrc` to persist across sessions.

## Multiple Kernel Versions

To work with multiple kernels:

```bash
/kernel/
├── linux-6.1.125/
├── linux-6.6.70/
├── linux-6.12.7/
└── linux -> linux-6.6.70   # Symlink to default
```

Switch versions by updating the symlink:

```bash
cd /kernel
ln -sfn linux-6.12.7 linux
```

## Building Against Host Kernel

If developing for your host system (not cross-compiling):

```bash
# Headers are typically at:
/lib/modules/$(uname -r)/build

# Build example:
make KERNEL_DIR=/lib/modules/$(uname -r)/build
```

## Adding a Driver to the Kernel Source Tree

All examples in this guide build out-of-tree (external modules). But production drivers live inside the kernel source tree. Here's how to add one.

### Step 1: Create Your Driver Directory

```bash
cd /kernel/linux/drivers/misc
mkdir my_driver
```

### Step 2: Add Source and Kconfig

Create `drivers/misc/my_driver/my_driver.c` (your driver code) and a Kconfig entry:

```bash
# drivers/misc/my_driver/Kconfig
config MY_DRIVER
    tristate "My example driver"
    help
      This is an example in-tree driver.
      Say M to build as a loadable module.
```

### Step 3: Add Makefile

```makefile
# drivers/misc/my_driver/Makefile
obj-$(CONFIG_MY_DRIVER) += my_driver.o
```

`obj-$(CONFIG_MY_DRIVER)` means: build as module when `CONFIG_MY_DRIVER=m`, build-in when `=y`, skip when `=n`.

### Step 4: Hook into Parent Kconfig and Makefile

```bash
# Append to drivers/misc/Kconfig (before the final "endmenu" if present)
echo 'source "drivers/misc/my_driver/Kconfig"' >> drivers/misc/Kconfig

# Append to drivers/misc/Makefile
echo 'obj-$(CONFIG_MY_DRIVER) += my_driver/' >> drivers/misc/Makefile
```

### Step 5: Configure and Build

```bash
# Enable via menuconfig (Device Drivers → Misc → My example driver → M)
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig

# Or enable directly in .config
echo "CONFIG_MY_DRIVER=m" >> .config
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig

# Build just your module
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- M=drivers/misc/my_driver
```

### Out-of-Tree vs In-Tree

| Aspect | Out-of-Tree | In-Tree |
|--------|------------|---------|
| Build | Separate Makefile with `M=` | Integrated with kernel build |
| Config | Always built | Controlled by Kconfig (`y`/`m`/`n`) |
| Distribution | Separate `.ko` file | Included in kernel packages |
| Upstream | Not possible | Required for mainline |
| Dependencies | Manual | Kconfig handles automatically |

{: .tip }
Develop out-of-tree for fast iteration, then move in-tree when preparing for upstream submission. See [Appendix F: Upstreaming]({% link appendices/upstreaming/index.md %}) for the full submission process.

## Troubleshooting

### "No rule to make target 'modules'"

The kernel isn't configured:

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules_prepare
```

### Missing Headers

Ensure `modules_prepare` completed:

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules_prepare
```

### Version Mismatch Warnings

When loading modules, kernel and module versions must match. For development, this is usually fine. For production, ensure exact version alignment.

## Kernel Documentation

The kernel includes extensive documentation:

```bash
# Read a documentation file
less /kernel/linux/Documentation/driver-api/index.rst

# Generate HTML docs (optional)
make htmldocs
# Output in Documentation/output/
```

## Next Steps

With kernel sources ready, let's [write your first module]({% link part1/04-first-module.md %}).
