---
layout: default
title: "2.4 Data Types"
parent: "Part 2: Kernel Fundamentals"
nav_order: 4
---

# Kernel Data Types

The kernel uses specific data types for portability and clarity. Understanding these is crucial for writing correct driver code.

## Fixed-Width Integer Types

Always use fixed-width types when size matters (hardware registers, protocols):

```c
#include <linux/types.h>

u8   val8;    /* Unsigned 8-bit  (0 to 255) */
u16  val16;   /* Unsigned 16-bit (0 to 65535) */
u32  val32;   /* Unsigned 32-bit */
u64  val64;   /* Unsigned 64-bit */

s8   sval8;   /* Signed 8-bit  (-128 to 127) */
s16  sval16;  /* Signed 16-bit */
s32  sval32;  /* Signed 32-bit */
s64  sval64;  /* Signed 64-bit */
```

### When to Use Fixed-Width Types

```c
/* Hardware register access - ALWAYS use fixed-width */
u32 reg_value = readl(base + CONTROL_REG);

/* Protocol data structures */
struct my_header {
        u8  version;
        u8  flags;
        u16 length;
        u32 sequence;
} __packed;

/* Loop counters, local variables - use int */
int i;
for (i = 0; i < count; i++)
        ...
```

## Size-Related Types

```c
size_t     len;    /* Size of objects (unsigned) */
ssize_t    ret;    /* Size or error (signed) */
loff_t     offset; /* File offset (64-bit) */
```

### Usage Example

```c
static ssize_t my_read(struct file *file, char __user *buf,
                       size_t count, loff_t *ppos)
{
        size_t remaining = data_len - *ppos;
        size_t to_copy = min(count, remaining);

        if (copy_to_user(buf, data + *ppos, to_copy))
                return -EFAULT;

        *ppos += to_copy;
        return to_copy;  /* ssize_t return: count or negative errno */
}
```

## Physical and DMA Addresses

```c
#include <linux/types.h>

phys_addr_t   phys;    /* Physical address */
dma_addr_t    dma;     /* DMA address */
resource_size_t res;   /* Resource size/address */
```

### Example: DMA Allocation

```c
void *virt;
dma_addr_t dma_handle;

virt = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);
if (!virt)
        return -ENOMEM;

/* dma_handle is the address to give to hardware */
writel(dma_handle, base + DMA_ADDR_REG);
```

## Boolean Type

```c
#include <linux/types.h>

bool enabled = true;
bool valid = false;

if (enabled)
        do_something();
```

## Pointer Annotations

### `__user` - User Space Pointer

```c
/* Never dereference directly! */
char __user *user_buf;

/* Use accessor functions */
if (copy_from_user(kernel_buf, user_buf, len))
        return -EFAULT;
```

### `__iomem` - I/O Memory Pointer

```c
void __iomem *base;

base = ioremap(phys_addr, size);
if (!base)
        return -ENOMEM;

/* Use I/O accessors */
u32 val = readl(base + OFFSET);
writel(new_val, base + OFFSET);
```

### `__percpu` - Per-CPU Data

```c
DEFINE_PER_CPU(int, my_counter);

/* Access current CPU's copy */
int val = get_cpu_var(my_counter);
put_cpu_var(my_counter);
```

## Endianness

Hardware may use different byte order than CPU:

```
Little-endian (x86, ARM default):
0x12345678 stored as: 78 56 34 12

Big-endian (network byte order):
0x12345678 stored as: 12 34 56 78
```

### Conversion Functions

```c
#include <linux/byteorder/generic.h>

/* CPU to little-endian */
u32 le = cpu_to_le32(value);
u16 le = cpu_to_le16(value);

/* Little-endian to CPU */
u32 cpu = le32_to_cpu(le_value);
u16 cpu = le16_to_cpu(le_value);

/* CPU to big-endian */
u32 be = cpu_to_be32(value);

/* Big-endian to CPU */
u32 cpu = be32_to_cpu(be_value);
```

### Annotated Types

```c
#include <linux/types.h>

__le16  le_val16;  /* Little-endian 16-bit */
__le32  le_val32;  /* Little-endian 32-bit */
__le64  le_val64;  /* Little-endian 64-bit */

__be16  be_val16;  /* Big-endian 16-bit */
__be32  be_val32;  /* Big-endian 32-bit */
__be64  be_val64;  /* Big-endian 64-bit */
```

### Example: Hardware Register Structure

```c
struct device_registers {
        __le32 control;    /* Little-endian hardware */
        __le32 status;
        __le32 data;
} __packed;

static void write_control(struct device_registers __iomem *regs, u32 val)
{
        /* Convert CPU to little-endian, then write */
        writel(val, &regs->control);  /* writel handles conversion */
}

static u32 read_status(struct device_registers __iomem *regs)
{
        /* readl reads and converts to CPU endianness */
        return readl(&regs->status);
}
```

{: .note }
`readl()` and `writel()` handle endianness automatically for memory-mapped I/O on most architectures.

## Structure Packing

### `__packed` Attribute

Remove padding between structure members:

```c
/* Without __packed: may have padding */
struct normal {
        u8  a;      /* 1 byte */
                    /* 3 bytes padding */
        u32 b;      /* 4 bytes */
};                  /* Total: 8 bytes */

/* With __packed: no padding */
struct packed {
        u8  a;      /* 1 byte */
        u32 b;      /* 4 bytes */
} __packed;         /* Total: 5 bytes */
```

### `__aligned` Attribute

```c
struct aligned_data {
        u32 value;
} __aligned(64);  /* 64-byte alignment */
```

## Atomic Types

For concurrent access without locks:

```c
#include <linux/atomic.h>

atomic_t counter = ATOMIC_INIT(0);

atomic_inc(&counter);          /* counter++ */
atomic_dec(&counter);          /* counter-- */
int val = atomic_read(&counter);
atomic_set(&counter, 10);

/* For 64-bit atomic operations */
atomic64_t big_counter = ATOMIC64_INIT(0);
```

## Time Types

```c
#include <linux/time.h>
#include <linux/ktime.h>

ktime_t now = ktime_get();           /* High-resolution time */
time64_t seconds = ktime_get_real_seconds();  /* Wall clock */

unsigned long jiffies_val = jiffies; /* Tick counter */
```

## Error Pointers

Encode errors in pointer values:

```c
#include <linux/err.h>

void *ptr;

ptr = some_allocation();
if (IS_ERR(ptr))
        return PTR_ERR(ptr);  /* Extract error code */

/* Create error pointer */
return ERR_PTR(-ENOMEM);
```

## Summary

| Use Case | Type |
|----------|------|
| Hardware registers | `u8`, `u16`, `u32`, `u64` |
| Sizes and counts | `size_t`, `ssize_t` |
| File offsets | `loff_t` |
| Physical addresses | `phys_addr_t` |
| DMA addresses | `dma_addr_t` |
| User pointers | `__user` annotation |
| I/O memory | `__iomem` annotation |
| Little-endian | `__le16`, `__le32` with `cpu_to_le*` |
| Big-endian | `__be16`, `__be32` with `cpu_to_be*` |

## Next

Learn about [error handling patterns]({% link part2/05-error-handling.md %}) used throughout the kernel.
