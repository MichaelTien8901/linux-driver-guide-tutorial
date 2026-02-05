---
layout: default
title: "5.1 Memory Allocators"
parent: "Part 5: Memory Management"
nav_order: 1
---

# Memory Allocators

The kernel provides several memory allocation functions, each designed for different use cases. Choosing the right one is essential for correctness and performance.

## kmalloc: General Purpose Allocator

The most common allocator, similar to user-space `malloc()`:

```c
#include <linux/slab.h>

void *ptr;

/* Allocate memory */
ptr = kmalloc(size, GFP_KERNEL);
if (!ptr)
    return -ENOMEM;

/* Use the memory */
/* ... */

/* Free when done */
kfree(ptr);
```

### kmalloc Characteristics

- Returns physically contiguous memory
- Fast for small allocations (uses slab cache)
- Maximum size typically ~128KB (depends on architecture)
- Returns actual size >= requested (rounded to power of 2)

### Common kmalloc Variants

```c
/* kmalloc with zeroing */
ptr = kzalloc(size, GFP_KERNEL);  /* Zeroed memory */

/* kmalloc for arrays */
ptr = kmalloc_array(n, elem_size, GFP_KERNEL);  /* Overflow-safe */

/* kzalloc for arrays */
ptr = kcalloc(n, elem_size, GFP_KERNEL);  /* Zeroed array */

/* Reallocate */
new_ptr = krealloc(ptr, new_size, GFP_KERNEL);

/* Duplicate string */
new_str = kstrdup(str, GFP_KERNEL);
new_str = kstrndup(str, max_len, GFP_KERNEL);

/* Duplicate memory */
new_ptr = kmemdup(ptr, size, GFP_KERNEL);
```

### Example: Basic Allocation

```c
struct my_data {
    int id;
    char name[32];
    void *buffer;
};

static struct my_data *create_my_data(int id, const char *name)
{
    struct my_data *data;

    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if (!data)
        return NULL;

    data->id = id;
    strscpy(data->name, name, sizeof(data->name));

    data->buffer = kmalloc(1024, GFP_KERNEL);
    if (!data->buffer) {
        kfree(data);
        return NULL;
    }

    return data;
}

static void destroy_my_data(struct my_data *data)
{
    if (!data)
        return;

    kfree(data->buffer);
    kfree(data);
}
```

## vmalloc: Large Allocations

For allocations larger than what kmalloc can handle:

```c
#include <linux/vmalloc.h>

void *ptr;

/* Allocate virtually contiguous memory */
ptr = vmalloc(large_size);
if (!ptr)
    return -ENOMEM;

/* Use the memory */
/* ... */

/* Free when done */
vfree(ptr);
```

### vmalloc Characteristics

- Memory is virtually contiguous, not physically
- Can allocate much larger regions
- Slower than kmalloc
- Cannot be used for DMA (not physically contiguous)
- Always sleeps (cannot use in atomic context)

### vmalloc Variants

```c
/* Zeroed vmalloc */
ptr = vzalloc(size);

/* With specific NUMA node */
ptr = vmalloc_node(size, node);
ptr = vzalloc_node(size, node);

/* For executable code */
ptr = __vmalloc(size, GFP_KERNEL);
```

## kvmalloc: Flexible Allocator

Tries kmalloc first, falls back to vmalloc:

```c
#include <linux/mm.h>

/* Best of both worlds */
ptr = kvmalloc(size, GFP_KERNEL);

/* With zeroing */
ptr = kvzalloc(size, GFP_KERNEL);

/* Array allocation */
ptr = kvmalloc_array(n, size, GFP_KERNEL);
ptr = kvcalloc(n, size, GFP_KERNEL);

/* Free (works for either) */
kvfree(ptr);
```

### When to Use kvmalloc

- Size might be small or large
- Physical contiguity not required
- Good default choice for flexible sizes

## Page Allocator

For page-aligned allocations:

```c
#include <linux/gfp.h>

/* Allocate 2^order pages */
struct page *page = alloc_pages(GFP_KERNEL, order);
if (!page)
    return -ENOMEM;

/* Get virtual address */
void *addr = page_address(page);

/* Free pages */
__free_pages(page, order);

/* Or simpler interface */
unsigned long addr = __get_free_pages(GFP_KERNEL, order);
free_pages(addr, order);

/* Single page */
unsigned long addr = __get_free_page(GFP_KERNEL);
free_page(addr);

/* Zeroed page */
unsigned long addr = get_zeroed_page(GFP_KERNEL);
```

### Page Order

| Order | Pages | Size (4KB pages) |
|-------|-------|------------------|
| 0 | 1 | 4 KB |
| 1 | 2 | 8 KB |
| 2 | 4 | 16 KB |
| 3 | 8 | 32 KB |
| 10 | 1024 | 4 MB |

## Comparison Summary

| Feature | kmalloc | vmalloc | kvmalloc |
|---------|---------|---------|----------|
| Max size | ~128KB | GBs | GBs |
| Physically contiguous | Yes | No | Maybe |
| Speed | Fast | Slower | Depends |
| DMA-able | Yes | No | Maybe |
| Atomic context | With GFP_ATOMIC | No | No |

## Size and Alignment

### Checking Actual Size

```c
size_t actual = ksize(ptr);  /* Actual allocated size */
```

### Aligned Allocation

```c
/* kmalloc returns naturally aligned memory */
/* For larger alignments: */
ptr = kmalloc(size + alignment - 1, GFP_KERNEL);
aligned_ptr = PTR_ALIGN(ptr, alignment);

/* Or use page allocator for page alignment */
```

## Error Handling

```c
/* Always check for allocation failure */
ptr = kmalloc(size, GFP_KERNEL);
if (!ptr) {
    pr_err("Failed to allocate %zu bytes\n", size);
    return -ENOMEM;
}

/* Safe pattern for structure with members */
struct complex {
    char *name;
    void *buffer;
    int *array;
};

struct complex *create_complex(int array_size)
{
    struct complex *c;

    c = kzalloc(sizeof(*c), GFP_KERNEL);
    if (!c)
        return NULL;

    c->name = kstrdup("test", GFP_KERNEL);
    if (!c->name)
        goto err_name;

    c->buffer = kmalloc(4096, GFP_KERNEL);
    if (!c->buffer)
        goto err_buffer;

    c->array = kmalloc_array(array_size, sizeof(int), GFP_KERNEL);
    if (!c->array)
        goto err_array;

    return c;

err_array:
    kfree(c->buffer);
err_buffer:
    kfree(c->name);
err_name:
    kfree(c);
    return NULL;
}
```

## Memory Debugging

Enable kernel config options for debugging:

```bash
CONFIG_DEBUG_SLAB=y
CONFIG_SLUB_DEBUG=y
CONFIG_KASAN=y  # Kernel Address Sanitizer
```

Runtime debugging:

```bash
# Enable SLUB debugging at boot
slub_debug=FZPU

# For specific cache
slub_debug=FZPU,kmalloc-64
```

## Best Practices

### Do

```c
/* DO: Use sizeof on the variable, not the type */
struct my_struct *p = kmalloc(sizeof(*p), GFP_KERNEL);

/* DO: Use kzalloc when you need zeroed memory */
p = kzalloc(sizeof(*p), GFP_KERNEL);

/* DO: Use array functions for arrays */
arr = kmalloc_array(count, sizeof(*arr), GFP_KERNEL);

/* DO: Check allocation success */
if (!ptr)
    return -ENOMEM;
```

### Don't

```c
/* DON'T: Use sizeof on the type */
struct my_struct *p = kmalloc(sizeof(struct my_struct), flags);

/* DON'T: Multiply sizes manually (overflow risk) */
arr = kmalloc(count * sizeof(*arr), flags);

/* DON'T: Forget to free on error paths */
/* DON'T: Double free */
/* DON'T: Use after free */
```

## Summary

- Use `kmalloc()`/`kzalloc()` for general small allocations
- Use `vmalloc()` for large allocations that don't need physical contiguity
- Use `kvmalloc()` when size varies
- Always check return values
- Use array variants for overflow protection
- Match allocator with context (GFP flags)

## Next

Learn about [GFP flags]({% link part5/02-gfp-flags.md %}) to control allocation behavior.
