---
layout: default
title: "Appendix D: Security"
nav_order: 20
---

# Appendix D: Security Considerations

Security checklist for driver developers.

## Input Validation

**Always validate user input:**

```c
/* Validate sizes */
if (count > MAX_BUFFER_SIZE)
    return -EINVAL;

/* Validate offsets */
if (offset >= device_size)
    return -EINVAL;

/* Use safe copy functions */
if (copy_from_user(kbuf, ubuf, count))
    return -EFAULT;
```

## Capability Checks

For privileged operations:

```c
/* Check for raw I/O access */
if (!capable(CAP_SYS_RAWIO))
    return -EPERM;

/* Check for admin operations */
if (!capable(CAP_SYS_ADMIN))
    return -EPERM;
```

Common capabilities:
- `CAP_SYS_ADMIN`: General admin
- `CAP_SYS_RAWIO`: Raw I/O access
- `CAP_NET_ADMIN`: Network configuration

## Preventing Information Leaks

```c
/* Zero memory before use */
buf = kzalloc(size, GFP_KERNEL);

/* Clear stack variables */
struct my_struct data = {};

/* Don't leak kernel addresses to userspace */
/* Bad: return sprintf(buf, "%p", ptr); */
/* Good: return sprintf(buf, "%px", ptr); */  /* Hashed */
```

## Secure DMA

Use IOMMU when available:

```c
/* Set DMA mask - also enables IOMMU protection */
ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));

/* Use DMA API, not direct physical addresses */
dma_addr_t dma_handle;
void *buf = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);
```

## Integer Overflow

```c
/* Check for overflow */
if (check_mul_overflow(count, elem_size, &total))
    return -EOVERFLOW;

/* Use safe allocation */
buf = kmalloc_array(count, elem_size, GFP_KERNEL);
```

## Security Checklist

- [ ] Validate all user input sizes and offsets
- [ ] Use `copy_from_user()`/`copy_to_user()`
- [ ] Check capabilities for privileged ops
- [ ] Zero buffers before use
- [ ] Don't leak kernel pointers
- [ ] Use IOMMU/DMA API
- [ ] Check integer overflow

## Further Reading

- [Kernel Security](https://docs.kernel.org/security/index.html)
- [KSPP Guidelines](https://kernsec.org/wiki/index.php/Kernel_Self_Protection_Project)
