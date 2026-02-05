---
layout: default
title: "1.4 First Module"
parent: "Part 1: Getting Started"
nav_order: 4
---

# Your First Kernel Module

Let's write the classic "Hello World" kernel module. This introduces the basic structure every kernel module follows.

## Module Anatomy

Every kernel module needs:

1. **Initialization function**: Called when module loads
2. **Cleanup function**: Called when module unloads
3. **Module metadata**: License, author, description

## Hello World Module

Create `hello.c`:

```c
// SPDX-License-Identifier: GPL-2.0
/*
 * hello.c - A simple "Hello World" kernel module
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

/* Module initialization function */
static int __init hello_init(void)
{
    pr_info("Hello, World! Module loaded.\n");
    return 0;  /* Success */
}

/* Module cleanup function */
static void __exit hello_exit(void)
{
    pr_info("Goodbye, World! Module unloaded.\n");
}

/* Register init and exit functions */
module_init(hello_init);
module_exit(hello_exit);

/* Module metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Hello World module");
MODULE_VERSION("1.0");
```

## Code Breakdown

### Headers

```c
#include <linux/init.h>    /* __init, __exit macros */
#include <linux/module.h>  /* MODULE_* macros */
#include <linux/kernel.h>  /* pr_info, pr_err, etc. */
```

These are kernel headers, not user-space headers. They're located in the kernel source tree.

### The `__init` Macro

```c
static int __init hello_init(void)
```

- `static`: Function is local to this file
- `__init`: Memory is freed after initialization (optimization)
- Returns `0` on success, negative errno on failure

### The `__exit` Macro

```c
static void __exit hello_exit(void)
```

- `__exit`: Marks code that's only needed for cleanup
- Not included when module is built-in to kernel

### Module Registration

```c
module_init(hello_init);
module_exit(hello_exit);
```

These macros tell the kernel which functions to call on load/unload.

### Logging with pr_info

```c
pr_info("Hello, World!\n");
```

Kernel logging functions by severity:

| Function | Level | Use Case |
|----------|-------|----------|
| `pr_emerg()` | 0 | System unusable |
| `pr_alert()` | 1 | Immediate action needed |
| `pr_crit()` | 2 | Critical condition |
| `pr_err()` | 3 | Error condition |
| `pr_warn()` | 4 | Warning |
| `pr_notice()` | 5 | Normal but significant |
| `pr_info()` | 6 | Informational |
| `pr_debug()` | 7 | Debug (only if enabled) |

### Module License

```c
MODULE_LICENSE("GPL");
```

{: .important }
Using `"GPL"` is required to access most kernel APIs. Non-GPL modules have limited functionality and will "taint" the kernel.

## The Makefile

Create `Makefile`:

```makefile
# Module name (without .ko)
obj-m := hello.o

# Kernel build directory
KERNEL_DIR ?= /kernel/linux

# Current directory
PWD := $(shell pwd)

# Default target
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

.PHONY: all clean
```

### Understanding the Makefile

- `obj-m := hello.o`: Build `hello.o` as a module (resulting in `hello.ko`)
- `-C $(KERNEL_DIR)`: Change to kernel directory
- `M=$(PWD)`: Tell kernel build system where our module is
- `modules`: Build external modules target

## Building the Module

### Cross-compile for ARM64

```bash
cd /workspace/examples/part1/hello-world
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KERNEL_DIR=/kernel/linux
```

### Build for Host

```bash
make KERNEL_DIR=/lib/modules/$(uname -r)/build
```

### Expected Output

```
make -C /kernel/linux M=/workspace/examples/part1/hello-world modules
make[1]: Entering directory '/kernel/linux'
  CC [M]  /workspace/examples/part1/hello-world/hello.o
  MODPOST /workspace/examples/part1/hello-world/Module.symvers
  CC [M]  /workspace/examples/part1/hello-world/hello.mod.o
  LD [M]  /workspace/examples/part1/hello-world/hello.ko
make[1]: Leaving directory '/kernel/linux'
```

### Build Artifacts

```
hello-world/
├── hello.c          # Source code
├── Makefile         # Build rules
├── hello.ko         # Kernel module (this is what we load!)
├── hello.o          # Object file
├── hello.mod        # Module metadata
├── hello.mod.c      # Generated module info
├── hello.mod.o      # Module info object
├── Module.symvers   # Symbol versions
└── modules.order    # Module ordering
```

## Loading and Testing

On a running Linux system (or in QEMU - see next chapter):

### Load the Module

```bash
sudo insmod hello.ko
```

### View Kernel Log

```bash
dmesg | tail
# [12345.678901] Hello, World! Module loaded.
```

### List Loaded Modules

```bash
lsmod | grep hello
# hello                  16384  0
```

### View Module Info

```bash
modinfo hello.ko
# filename:       /path/to/hello.ko
# version:        1.0
# description:    A simple Hello World module
# author:         Your Name
# license:        GPL
# ...
```

### Unload the Module

```bash
sudo rmmod hello
dmesg | tail
# [12345.789012] Goodbye, World! Module unloaded.
```

## Common Errors

### "Invalid module format"

Kernel version mismatch. Rebuild against the running kernel:

```bash
make KERNEL_DIR=/lib/modules/$(uname -r)/build
```

### "Unknown symbol"

Module uses a symbol not exported by kernel. Check `MODULE_LICENSE("GPL")`.

### "Permission denied"

Module loading requires root. Use `sudo`.

## Module Parameters

Modules can accept parameters at load time. We'll cover this in detail in Part 2, but here's a preview:

```c
static int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of times to print");

static int __init hello_init(void)
{
    int i;
    for (i = 0; i < count; i++)
        pr_info("Hello %d!\n", i);
    return 0;
}
```

Load with parameter:

```bash
sudo insmod hello.ko count=5
```

## Full Example

The complete example is available at:

```
docs/examples/part1/hello-world/
├── hello.c
├── Makefile
└── README.md
```

## Next Steps

Before we can test on real hardware, let's set up [QEMU for module testing]({% link part1/05-qemu-testing.md %}).
