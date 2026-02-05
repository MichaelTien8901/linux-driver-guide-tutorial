---
layout: default
title: "2.6 Kernel API"
parent: "Part 2: Kernel Fundamentals"
nav_order: 6
---

# Common Kernel API and Macros

The kernel provides many helper macros and functions. Knowing these will make your code cleaner and more idiomatic.

## Container Of

Get the parent structure from a member pointer:

```c
#include <linux/container_of.h>

struct my_device {
        int id;
        struct cdev cdev;  /* Embedded cdev */
        char name[32];
};

/* In a cdev callback, we get a pointer to cdev.
 * container_of gives us the parent my_device. */
static int my_open(struct inode *inode, struct file *file)
{
        struct cdev *cdev = inode->i_cdev;
        struct my_device *dev = container_of(cdev, struct my_device, cdev);

        pr_info("Opening device %s (id=%d)\n", dev->name, dev->id);
        file->private_data = dev;
        return 0;
}
```

### How It Works

```c
container_of(ptr, type, member)

/* Equivalent to: */
(type *)((char *)(ptr) - offsetof(type, member))
```

## Array Size

Get the number of elements in a static array:

```c
#include <linux/array_size.h>

static const char *names[] = { "foo", "bar", "baz" };

for (i = 0; i < ARRAY_SIZE(names); i++)
        pr_info("Name: %s\n", names[i]);
```

{: .warning }
Only works with actual arrays, not pointers! `ARRAY_SIZE(pointer)` gives wrong results.

## Bit Operations

```c
#include <linux/bits.h>

/* Create a bitmask */
BIT(n)        /* 1 << n */
BIT_ULL(n)    /* 1ULL << n (64-bit) */

/* Mask of bits 0 to n-1 */
GENMASK(high, low)
GENMASK_ULL(high, low)

/* Examples */
#define STATUS_READY   BIT(0)    /* 0x01 */
#define STATUS_ERROR   BIT(1)    /* 0x02 */
#define STATUS_BUSY    BIT(2)    /* 0x04 */

#define DATA_MASK      GENMASK(7, 0)     /* 0xFF */
#define ADDR_MASK      GENMASK(31, 12)   /* 0xFFFFF000 */
```

### Bitfield Extraction

```c
#include <linux/bitfield.h>

#define FIELD_A  GENMASK(7, 0)
#define FIELD_B  GENMASK(15, 8)

u32 reg = readl(base + REG_OFFSET);

/* Extract field values */
u8 a = FIELD_GET(FIELD_A, reg);
u8 b = FIELD_GET(FIELD_B, reg);

/* Prepare field values */
reg = FIELD_PREP(FIELD_A, 0x12) | FIELD_PREP(FIELD_B, 0x34);
writel(reg, base + REG_OFFSET);
```

## Min and Max

```c
#include <linux/minmax.h>

/* Type-safe min/max */
int a = 5, b = 10;
int smaller = min(a, b);  /* 5 */
int larger = max(a, b);   /* 10 */

/* Clamping to a range */
int val = clamp(x, min_val, max_val);

/* With explicit type */
u32 result = min_t(u32, value1, value2);

/* Three-way min/max */
int smallest = min3(a, b, c);
```

## Alignment

```c
#include <linux/align.h>

/* Round up to alignment */
ALIGN(x, a)

/* Round down to alignment */
ALIGN_DOWN(x, a)

/* Check if aligned */
IS_ALIGNED(x, a)

/* Examples */
size_t aligned = ALIGN(17, 8);     /* 24 */
size_t down = ALIGN_DOWN(17, 8);   /* 16 */
bool ok = IS_ALIGNED(16, 8);       /* true */
```

## Likely and Unlikely

Branch prediction hints:

```c
#include <linux/compiler.h>

/* Tell compiler this branch is usually taken */
if (likely(condition)) {
        /* Common case */
}

/* Tell compiler this branch is rarely taken */
if (unlikely(error)) {
        /* Error handling */
}
```

Use sparingly - only when you know the branch behavior and it's performance-critical.

## String Functions

The kernel has its own string functions (not libc):

```c
#include <linux/string.h>

/* Copying */
strscpy(dst, src, size);        /* Safe strcpy */
strscpy_pad(dst, src, size);    /* Pad with zeros */

/* Comparison */
int cmp = strcmp(s1, s2);
int cmp = strncmp(s1, s2, n);
int cmp = strcasecmp(s1, s2);   /* Case-insensitive */

/* Searching */
char *p = strstr(haystack, needle);
char *p = strchr(str, 'c');

/* Length */
size_t len = strlen(str);
size_t len = strnlen(str, max);

/* Memory operations */
memset(ptr, 0, size);
memcpy(dst, src, size);
memmove(dst, src, size);        /* Handles overlap */
int cmp = memcmp(p1, p2, size);
```

{: .important }
Never use `strcpy()` or `strcat()` in kernel code. Use `strscpy()` instead.

## Memory Allocation

```c
#include <linux/slab.h>

/* Basic allocation */
ptr = kmalloc(size, GFP_KERNEL);
ptr = kzalloc(size, GFP_KERNEL);    /* Zero-initialized */

/* Array allocation */
ptr = kmalloc_array(n, element_size, GFP_KERNEL);
ptr = kcalloc(n, element_size, GFP_KERNEL);  /* Zeroed */

/* Freeing */
kfree(ptr);

/* String duplication */
new_str = kstrdup(str, GFP_KERNEL);
new_str = kstrndup(str, max_len, GFP_KERNEL);
```

## Printf-like Functions

```c
#include <linux/kernel.h>

/* Kernel logging */
pr_info("Message: %d\n", value);
pr_err("Error: %s\n", msg);
pr_debug("Debug: %p\n", ptr);   /* Only if DEBUG defined */

/* Device-aware logging */
dev_info(dev, "Device ready\n");
dev_err(dev, "Error: %d\n", ret);
dev_dbg(dev, "Register: 0x%08x\n", reg);

/* Format to buffer */
int len = snprintf(buf, size, "Value: %d", val);
int len = scnprintf(buf, size, "Value: %d", val);  /* Returns chars written, not what would be */
```

### Kernel Printf Specifiers

| Specifier | Meaning |
|-----------|---------|
| `%d`, `%i` | Signed decimal |
| `%u` | Unsigned decimal |
| `%x`, `%X` | Hexadecimal |
| `%08x` | Hex with leading zeros |
| `%p` | Pointer (hashed for security) |
| `%px` | Raw pointer (debug only) |
| `%pK` | Pointer (kernel restriction aware) |
| `%pS` | Symbol name from pointer |
| `%pF` | Function name from pointer |
| `%*ph` | Hex dump of buffer |
| `%pI4` | IPv4 address |
| `%pI6` | IPv6 address |
| `%pM` | MAC address |
| `%pUb` | UUID |

```c
/* Examples */
pr_info("Pointer: %p\n", ptr);         /* Hashed address */
pr_info("Function: %pS\n", func_ptr);  /* "my_function+0x10" */
pr_info("MAC: %pM\n", mac_addr);       /* "00:11:22:33:44:55" */
pr_info("Hex: %*ph\n", 16, buffer);    /* "01 02 03 04..." */
```

## Time and Delays

```c
#include <linux/delay.h>

/* Busy-wait delays (use sparingly) */
udelay(10);       /* Microseconds */
mdelay(1);        /* Milliseconds (avoid if possible) */

/* Sleeping delays (process context only) */
usleep_range(min_us, max_us);  /* Preferred for us delays */
msleep(10);                    /* Milliseconds */
ssleep(1);                     /* Seconds */
```

```c
#include <linux/jiffies.h>

/* Timeout values */
unsigned long timeout = jiffies + msecs_to_jiffies(100);

/* Check if timeout expired */
if (time_after(jiffies, timeout))
        return -ETIMEDOUT;
```

## List Operations

```c
#include <linux/list.h>

struct my_item {
        int value;
        struct list_head list;  /* Embed list node */
};

/* Initialize list head */
LIST_HEAD(my_list);
/* Or dynamically: */
struct list_head my_list;
INIT_LIST_HEAD(&my_list);

/* Add items */
list_add(&item->list, &my_list);       /* Add at head */
list_add_tail(&item->list, &my_list);  /* Add at tail */

/* Remove items */
list_del(&item->list);

/* Iterate */
struct my_item *item;
list_for_each_entry(item, &my_list, list) {
        pr_info("Value: %d\n", item->value);
}

/* Safe iteration (allows removal) */
struct my_item *tmp;
list_for_each_entry_safe(item, tmp, &my_list, list) {
        list_del(&item->list);
        kfree(item);
}
```

## Mutex and Spinlock Helpers

```c
#include <linux/mutex.h>
#include <linux/spinlock.h>

/* Static initialization */
DEFINE_MUTEX(my_mutex);
DEFINE_SPINLOCK(my_lock);

/* Dynamic initialization */
struct mutex lock;
mutex_init(&lock);

struct spinlock lock;
spin_lock_init(&lock);
```

## Summary

| Macro/Function | Purpose |
|----------------|---------|
| `container_of` | Get parent structure from member |
| `ARRAY_SIZE` | Array element count |
| `BIT(n)` | Create bitmask |
| `GENMASK` | Create bit range mask |
| `FIELD_GET/PREP` | Extract/prepare bitfields |
| `min/max/clamp` | Safe comparisons |
| `ALIGN` | Memory alignment |
| `likely/unlikely` | Branch hints |
| `strscpy` | Safe string copy |
| `kzalloc` | Allocate zeroed memory |
| `pr_*` / `dev_*` | Kernel logging |
| `LIST_HEAD` | Linked list operations |

## Next Steps

You've completed Part 2! Continue to [Part 3: Character Device Drivers]({% link part3/index.md %}) to build your first real driver.
