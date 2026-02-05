---
layout: default
title: "13.2 Driver Skeleton"
parent: "Part 13: Block Device Drivers"
nav_order: 2
---

# Block Driver Skeleton

This chapter shows the minimal blk-mq driver structure.

## Complete Skeleton

```c
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#define SECTOR_SIZE 512
#define DEVICE_SIZE (16 * 1024 * 1024)  /* 16 MB */

struct my_device {
    struct gendisk *disk;
    struct blk_mq_tag_set tag_set;
    void *data;  /* Device storage (for RAM disk) */
};

static struct my_device my_dev;

/* ============ Request Processing ============ */

static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx,
                                 const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct my_device *dev = rq->q->queuedata;
    struct req_iterator iter;
    struct bio_vec bvec;
    sector_t sector = blk_rq_pos(rq);

    blk_mq_start_request(rq);

    /* Process each segment */
    rq_for_each_segment(bvec, rq, iter) {
        void *buf = page_address(bvec.bv_page) + bvec.bv_offset;
        size_t offset = sector << SECTOR_SHIFT;

        if (rq_data_dir(rq))  /* Write */
            memcpy(dev->data + offset, buf, bvec.bv_len);
        else  /* Read */
            memcpy(buf, dev->data + offset, bvec.bv_len);

        sector += bvec.bv_len >> SECTOR_SHIFT;
    }

    blk_mq_end_request(rq, BLK_STS_OK);
    return BLK_STS_OK;
}

static const struct blk_mq_ops my_mq_ops = {
    .queue_rq = my_queue_rq,
};

/* ============ Module Init/Exit ============ */

static int __init my_init(void)
{
    int err;

    /* Allocate storage */
    my_dev.data = vzalloc(DEVICE_SIZE);
    if (!my_dev.data)
        return -ENOMEM;

    /* Set up tag set */
    my_dev.tag_set.ops = &my_mq_ops;
    my_dev.tag_set.nr_hw_queues = 1;
    my_dev.tag_set.queue_depth = 128;
    my_dev.tag_set.numa_node = NUMA_NO_NODE;
    my_dev.tag_set.flags = BLK_MQ_F_SHOULD_MERGE;

    err = blk_mq_alloc_tag_set(&my_dev.tag_set);
    if (err)
        goto free_data;

    /* Allocate disk and queue */
    my_dev.disk = blk_mq_alloc_disk(&my_dev.tag_set, &my_dev);
    if (IS_ERR(my_dev.disk)) {
        err = PTR_ERR(my_dev.disk);
        goto free_tag_set;
    }

    /* Configure disk */
    my_dev.disk->major = 0;  /* Dynamic major */
    my_dev.disk->first_minor = 0;
    my_dev.disk->minors = 1;
    snprintf(my_dev.disk->disk_name, DISK_NAME_LEN, "myblk0");
    set_capacity(my_dev.disk, DEVICE_SIZE / SECTOR_SIZE);

    /* Store device pointer for queue_rq */
    my_dev.disk->queue->queuedata = &my_dev;

    /* Make visible */
    err = add_disk(my_dev.disk);
    if (err)
        goto cleanup_disk;

    pr_info("Block device registered: /dev/%s (%lu MB)\n",
            my_dev.disk->disk_name, DEVICE_SIZE / (1024 * 1024));
    return 0;

cleanup_disk:
    put_disk(my_dev.disk);
free_tag_set:
    blk_mq_free_tag_set(&my_dev.tag_set);
free_data:
    vfree(my_dev.data);
    return err;
}

static void __exit my_exit(void)
{
    del_gendisk(my_dev.disk);
    put_disk(my_dev.disk);
    blk_mq_free_tag_set(&my_dev.tag_set);
    vfree(my_dev.data);
    pr_info("Block device unregistered\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
```

## Key Steps

### 1. Set Up Tag Set

```c
struct blk_mq_tag_set tag_set = {
    .ops = &my_mq_ops,
    .nr_hw_queues = 1,      /* Hardware queues (often 1 for simple) */
    .queue_depth = 128,     /* Max pending requests */
    .numa_node = NUMA_NO_NODE,
    .flags = BLK_MQ_F_SHOULD_MERGE,  /* Allow request merging */
};
blk_mq_alloc_tag_set(&tag_set);
```

### 2. Allocate Disk and Queue

```c
disk = blk_mq_alloc_disk(&tag_set, driver_data);
/* driver_data passed to queue_rq via bd->rq->q->queuedata */
```

### 3. Configure Disk

```c
disk->major = 0;  /* 0 = allocate dynamically */
snprintf(disk->disk_name, DISK_NAME_LEN, "myblk%d", id);
set_capacity(disk, size_in_sectors);
```

### 4. Register

```c
add_disk(disk);  /* Now visible as /dev/myblk0 */
```

### 5. Cleanup (in reverse order)

```c
del_gendisk(disk);
put_disk(disk);
blk_mq_free_tag_set(&tag_set);
```

## The queue_rq Callback

This is called for every request:

```c
static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx,
                                 const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;

    blk_mq_start_request(rq);  /* MUST call first */

    /* Get request info */
    sector_t pos = blk_rq_pos(rq);
    unsigned int bytes = blk_rq_bytes(rq);
    int write = rq_data_dir(rq);

    /* Process... */

    blk_mq_end_request(rq, BLK_STS_OK);  /* Or BLK_STS_IOERR */
    return BLK_STS_OK;
}
```

{: .warning }
> Always call `blk_mq_start_request()` before processing and `blk_mq_end_request()` when done.

## Return Values

| Value | Meaning |
|-------|---------|
| `BLK_STS_OK` | Success |
| `BLK_STS_IOERR` | I/O error |
| `BLK_STS_RESOURCE` | Retry later (queue busy) |

## Block Device Operations (Optional)

For custom open/release handling:

```c
static int my_open(struct gendisk *disk, blk_mode_t mode)
{
    /* Called when device is opened */
    return 0;
}

static void my_release(struct gendisk *disk)
{
    /* Called when device is closed */
}

static const struct block_device_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
};

/* In init: */
disk->fops = &my_fops;
```

## Summary

| Step | Function |
|------|----------|
| Allocate tag set | `blk_mq_alloc_tag_set()` |
| Allocate disk | `blk_mq_alloc_disk()` |
| Set capacity | `set_capacity(disk, sectors)` |
| Register | `add_disk()` |
| Process I/O | `queue_rq()` callback |
| Unregister | `del_gendisk()`, `put_disk()` |
| Free tag set | `blk_mq_free_tag_set()` |

## Further Reading

- [blk-mq API](https://docs.kernel.org/block/blk-mq.html) - Official documentation
- [Block Drivers](https://elixir.bootlin.com/linux/v6.6/source/drivers/block) - Kernel examples
