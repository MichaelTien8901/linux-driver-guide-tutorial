# Slab Cache (kmem_cache) Demo

Demonstrates using the slab allocator for efficient object allocation.

## Files

- `kmem_cache_demo.c` - Kernel module demonstrating kmem_cache
- `Makefile` - Build configuration

## Features Demonstrated

1. **Cache Creation**
   - `kmem_cache_create()` with constructor
   - `SLAB_HWCACHE_ALIGN` for performance

2. **Object Allocation**
   - `kmem_cache_alloc()` / `kmem_cache_free()`
   - Constructor for initialization

3. **Cache Destruction**
   - Proper cleanup order

## Building

```bash
make
```

## Testing

### Quick Test

```bash
make test
```

### Manual Testing

```bash
# Load module
sudo insmod kmem_cache_demo.ko

# View stats
cat /proc/kmem_cache_demo

# Allocate objects
echo "alloc my_data_1" | sudo tee /proc/kmem_cache_demo
echo "alloc my_data_2" | sudo tee /proc/kmem_cache_demo

# View objects
cat /proc/kmem_cache_demo

# Access an object (increment counter)
echo "access 0" | sudo tee /proc/kmem_cache_demo

# Free an object
echo "free 1" | sudo tee /proc/kmem_cache_demo

# Free all objects
echo "freeall" | sudo tee /proc/kmem_cache_demo

# Unload
sudo rmmod kmem_cache_demo
```

### View Slab Info

```bash
# System-wide slab info
cat /proc/slabinfo | grep demo

# Or using slabtop
sudo slabtop
```

## Output Example

```
Slab Cache Demo Statistics
==========================

Cache name: demo_objects
Object size: 128 bytes
Total allocated: 3
Total freed: 1
Currently active: 2

Active Objects:
  [2] data='object_three' age=1234 ms accesses=0
  [1] data='object_two' age=2345 ms accesses=5

Commands:
  alloc <data> - Allocate new object
  free <id>    - Free object by ID
  access <id>  - Increment access count
  freeall      - Free all objects
```

## Key Concepts

### Creating a Cache

```c
object_cache = kmem_cache_create(
    "demo_objects",           /* Name (shows in /proc/slabinfo) */
    sizeof(struct my_object), /* Object size */
    0,                        /* Alignment (0 = natural) */
    SLAB_HWCACHE_ALIGN,       /* Flags */
    object_ctor               /* Constructor function */
);
```

### Constructor Function

```c
static void object_ctor(void *ptr)
{
    struct my_object *obj = ptr;
    spin_lock_init(&obj->lock);
    INIT_LIST_HEAD(&obj->list);
}
```

The constructor is called once when memory is first allocated from the page allocator, not on every `kmem_cache_alloc()`. Objects are reused without re-running the constructor.

### Allocation and Free

```c
/* Allocate */
obj = kmem_cache_alloc(object_cache, GFP_KERNEL);

/* Free (returns to cache, not to system) */
kmem_cache_free(object_cache, obj);
```

### Cleanup

```c
/* Must free all objects first! */
kmem_cache_destroy(object_cache);
```

## When to Use Slab Caches

**Good use cases:**
- Frequently allocating/freeing same-size objects
- Need consistent, fast allocation times
- Want to track memory usage by type
- Objects benefit from constructor

**Use kmalloc instead when:**
- Object sizes vary
- Allocations are infrequent
- Simple, one-off allocations
