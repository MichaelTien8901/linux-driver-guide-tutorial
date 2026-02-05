# DMA Mapping Demo

Demonstrates DMA buffer allocation and mapping techniques.

## Files

- `dma_demo.c` - Kernel module demonstrating DMA APIs
- `Makefile` - Build configuration

## Features Demonstrated

1. **Coherent DMA**
   - `dma_alloc_coherent()` / `dma_free_coherent()`
   - Always synchronized between CPU and device
   - Used for descriptor rings, control structures

2. **Streaming DMA**
   - `dma_map_single()` / `dma_unmap_single()`
   - Manual synchronization required
   - Used for data buffers

3. **DMA Directions**
   - `DMA_TO_DEVICE` - CPU writes, device reads
   - `DMA_FROM_DEVICE` - Device writes, CPU reads
   - `DMA_BIDIRECTIONAL` - Both directions

4. **Synchronization**
   - `dma_sync_single_for_cpu()`
   - `dma_sync_single_for_device()`

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
sudo insmod dma_demo.ko

# View stats
cat /proc/dma_demo

# Write to coherent buffer
echo "cwrite my_data" | sudo tee /proc/dma_demo

# Read from coherent buffer
echo "cread" | sudo tee /proc/dma_demo

# Map streaming buffer for device read
echo "smap_to some_data" | sudo tee /proc/dma_demo

# Unmap streaming buffer
echo "sunmap_to" | sudo tee /proc/dma_demo

# Map for device write
echo "smap_from" | sudo tee /proc/dma_demo
echo "sunmap_from" | sudo tee /proc/dma_demo

# Check kernel log
dmesg | tail -20

# Unload
sudo rmmod dma_demo
```

## Output Example

```
DMA Demo Statistics
===================

Coherent Buffer:
  CPU address:  000000001234abcd
  DMA address:  0x00000000fedcba98
  Size:         1024 bytes
  Writes:       3
  Reads:        2

Streaming Buffer:
  CPU address:  000000005678efgh
  Size:         4096 bytes
  Mapped:       no
  Total maps:   5
  Total unmaps: 5
```

## Key Concepts

### Coherent DMA

```c
/* Allocate coherent buffer */
void *buf = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);

/* Use directly - always synchronized */
memcpy(buf, data, len);
wmb();  /* Ensure writes visible to device */

/* Free */
dma_free_coherent(dev, size, buf, dma_handle);
```

### Streaming DMA

```c
/* Map existing buffer */
dma_addr_t dma = dma_map_single(dev, buf, size, DMA_TO_DEVICE);
if (dma_mapping_error(dev, dma))
    return -ENOMEM;

/* Start DMA transfer... */

/* When done */
dma_unmap_single(dev, dma, size, DMA_TO_DEVICE);
```

### DMA Sync (for reusing mapped buffer)

```c
/* Device finished writing, CPU needs to read */
dma_sync_single_for_cpu(dev, dma, size, DMA_FROM_DEVICE);
/* Now CPU can read buffer */

/* CPU finished writing, device needs to read */
/* Fill buffer... */
dma_sync_single_for_device(dev, dma, size, DMA_TO_DEVICE);
/* Now device can read buffer */
```

## When to Use What

| Use Case | DMA Type |
|----------|----------|
| DMA descriptor ring | Coherent |
| Hardware register shadow | Coherent |
| Network packet data | Streaming |
| Block I/O data | Streaming |
| Video frame buffer | Coherent (or streaming) |

## Important Notes

1. **Always check `dma_mapping_error()`** after `dma_map_single()`
2. **Match direction** in map and unmap calls
3. **Don't access buffer** while it's mapped (owned by device)
4. **Set DMA mask** before allocating DMA memory
