---
layout: default
title: "13.1 Block Driver Concepts"
parent: "Part 13: Block Device Drivers"
nav_order: 1
---

# Block Driver Concepts

Understand these concepts before writing a block driver: sectors, gendisk, and the request flow.

## Sectors and Blocks

Storage is addressed in **sectors** (512 bytes, historically). The kernel may use larger **blocks** (typically 4KB) for efficiency.

```
Sector 0    Sector 1    Sector 2    Sector 3    ...
[512 bytes] [512 bytes] [512 bytes] [512 bytes]
|_______________________|_______________________|
        Block 0                 Block 1
       (4096 bytes)           (4096 bytes)
```

Your driver works with sector addresses. The constant `SECTOR_SHIFT` (9) converts between bytes and sectors:

```c
sector_t sector = byte_offset >> SECTOR_SHIFT;  /* bytes to sectors */
size_t bytes = num_sectors << SECTOR_SHIFT;     /* sectors to bytes */
```

## gendisk: Your Disk Identity

`gendisk` represents the disk itself:

```c
struct gendisk *disk = blk_alloc_disk(node_id);

/* Configure */
disk->major = my_major;           /* Major number */
disk->first_minor = 0;            /* First minor */
disk->minors = 1;                 /* Number of partitions + 1 */
disk->fops = &my_block_fops;      /* Optional: open/release */
set_capacity(disk, num_sectors);  /* Size in 512-byte sectors */
snprintf(disk->disk_name, DISK_NAME_LEN, "myblk%d", index);

/* Make visible */
add_disk(disk);
```

{: .note }
> `set_capacity()` takes sectors (512B units), not bytes. For a 1MB disk: `set_capacity(disk, 1024 * 1024 / 512)`.

## Request Queue and blk-mq

The **request queue** connects gendisk to your driver. With blk-mq, you provide:

```c
static const struct blk_mq_ops my_mq_ops = {
    .queue_rq = my_queue_rq,  /* Process one request */
};

struct blk_mq_tag_set tag_set = {
    .ops = &my_mq_ops,
    .nr_hw_queues = 1,        /* Number of hardware queues */
    .queue_depth = 128,       /* Max concurrent requests */
    .cmd_size = sizeof(struct my_cmd),  /* Per-request private data */
};

blk_mq_alloc_tag_set(&tag_set);
disk = blk_mq_alloc_disk(&tag_set, NULL);
```

## The Request

When the kernel wants I/O, it calls your `queue_rq` callback with a `struct request`:

```c
static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx,
                                 const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;

    /* Start processing */
    blk_mq_start_request(rq);

    /* Request info */
    sector_t sector = blk_rq_pos(rq);      /* Starting sector */
    unsigned int len = blk_rq_bytes(rq);   /* Total bytes */
    bool is_write = rq_data_dir(rq);       /* 0=read, 1=write */

    /* Process the request... */

    /* Complete */
    blk_mq_end_request(rq, BLK_STS_OK);
    return BLK_STS_OK;
}
```

## Iterating Over Segments

A request may contain multiple memory segments. Iterate with `rq_for_each_segment`:

```c
struct req_iterator iter;
struct bio_vec bvec;

rq_for_each_segment(bvec, rq, iter) {
    /* bvec describes one contiguous memory region */
    sector_t sector = iter.iter.bi_sector;
    void *buffer = page_address(bvec.bv_page) + bvec.bv_offset;
    unsigned int len = bvec.bv_len;

    if (rq_data_dir(rq))
        /* Write: copy from buffer to device */
        memcpy(device_mem + (sector << SECTOR_SHIFT), buffer, len);
    else
        /* Read: copy from device to buffer */
        memcpy(buffer, device_mem + (sector << SECTOR_SHIFT), len);
}
```

{: .tip }
> For simple cases (RAM disk), you can use `bio_for_each_segment` directly on each bio, or use `kmap_local_page()` for high memory pages.

## I/O Flow Summary

```mermaid
flowchart LR
    APP["write()"] --> FS["Filesystem"]
    FS --> PC["Page Cache"]
    PC --> BIO["bio"]
    BIO --> SCHED["Scheduler"]
    SCHED --> REQ["request"]
    REQ --> QR["queue_rq()"]
    QR --> YOUR["Your Code"]
    YOUR --> END["blk_mq_end_request()"]

    style QR fill:#8f8a73,stroke:#f9a825
    style YOUR fill:#8f8a73,stroke:#f9a825
```

1. Application writes → hits page cache
2. Dirty pages get flushed → creates bio
3. Block layer merges bios → creates request
4. Your `queue_rq()` is called
5. You process and call `blk_mq_end_request()`

## Summary

| Concept | Description |
|---------|-------------|
| Sector | 512-byte unit of addressing |
| gendisk | Represents the disk (capacity, name) |
| blk-mq | Multi-queue block interface |
| request | Batched I/O operation with sector + data |
| queue_rq | Your callback to process requests |

## Further Reading

- [Block Layer](https://docs.kernel.org/block/index.html) - Kernel documentation
- [blk-mq](https://docs.kernel.org/block/blk-mq.html) - Multi-queue details
