---
layout: default
title: "13.3 Block Device Operations"
parent: "Part 13: Block Device Drivers"
nav_order: 3
---

# Block Device Operations

This chapter covers `block_device_operations` for open/release, ioctl, and disk geometry.

## block_device_operations Structure

```c
#include <linux/blkdev.h>

static const struct block_device_operations my_fops = {
    .owner       = THIS_MODULE,
    .open        = my_open,
    .release     = my_release,
    .ioctl       = my_ioctl,
    .getgeo      = my_getgeo,
};

/* In init: */
disk->fops = &my_fops;
```

## Open and Release

Called when the block device is opened/closed:

```c
static int my_open(struct gendisk *disk, blk_mode_t mode)
{
    struct my_device *dev = disk->private_data;

    /* Check if device is ready */
    if (!dev->data)
        return -ENXIO;

    /* Track open count (optional) */
    atomic_inc(&dev->open_count);

    /* Check for exclusive access */
    if (mode & BLK_OPEN_EXCL) {
        if (atomic_read(&dev->open_count) > 1) {
            atomic_dec(&dev->open_count);
            return -EBUSY;
        }
    }

    return 0;
}

static void my_release(struct gendisk *disk)
{
    struct my_device *dev = disk->private_data;

    atomic_dec(&dev->open_count);

    /* Flush any cached data if needed */
}
```

{: .note }
> `release` is called when the last user closes the device, not on every `close()`.

## Disk Geometry

Report geometry for legacy tools and partitioning:

```c
static int my_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
    struct my_device *dev = bdev->bd_disk->private_data;
    sector_t capacity = get_capacity(bdev->bd_disk);

    /* Standard fake geometry for modern drives */
    geo->heads = 16;
    geo->sectors = 63;
    geo->cylinders = capacity / (geo->heads * geo->sectors);

    /* For small disks (< 16MB) */
    if (capacity < 16 * 2048) {
        geo->heads = 1;
        geo->sectors = 32;
        geo->cylinders = capacity / 32;
    }

    geo->start = 0;
    return 0;
}
```

Result of `fdisk -l /dev/myblk0`:
```
Disk /dev/myblk0: 16 MiB, ...
Geometry: 16 heads, 63 sectors/track, 32 cylinders
```

## IOCTL Operations

Handle custom device commands:

```c
#define MY_IOCTL_GET_SIZE _IOR('B', 1, unsigned long)
#define MY_IOCTL_CLEAR    _IO('B', 2)
#define MY_IOCTL_SET_RO   _IOW('B', 3, int)

static int my_ioctl(struct block_device *bdev, blk_mode_t mode,
                    unsigned int cmd, unsigned long arg)
{
    struct my_device *dev = bdev->bd_disk->private_data;
    void __user *argp = (void __user *)arg;

    switch (cmd) {
    case MY_IOCTL_GET_SIZE:
        return put_user(dev->size, (unsigned long __user *)argp);

    case MY_IOCTL_CLEAR:
        if (!(mode & BLK_OPEN_WRITE))
            return -EACCES;
        memset(dev->data, 0, dev->size);
        return 0;

    case MY_IOCTL_SET_RO:
    {
        int ro;
        if (get_user(ro, (int __user *)argp))
            return -EFAULT;
        set_disk_ro(bdev->bd_disk, ro);
        return 0;
    }

    default:
        return -ENOTTY;
    }
}
```

{: .tip }
> Return `-ENOTTY` for unknown ioctls, not `-EINVAL`. This follows convention and allows stacking.

## Read-Only Mode

Set or check read-only status:

```c
/* Set disk read-only */
set_disk_ro(disk, 1);

/* Check if read-only */
if (get_disk_ro(disk)) {
    /* Reject writes */
}

/* In queue_rq: enforce read-only */
if (rq_data_dir(rq) && get_disk_ro(rq->q->disk)) {
    blk_mq_end_request(rq, BLK_STS_IOERR);
    return BLK_STS_OK;
}
```

## Handling Media Changes

For removable media (like virtual disk images):

```c
static unsigned int my_check_events(struct gendisk *disk,
                                     unsigned int clearing)
{
    struct my_device *dev = disk->private_data;

    if (dev->media_changed) {
        dev->media_changed = false;
        return DISK_EVENT_MEDIA_CHANGE;
    }
    return 0;
}

static int my_revalidate_disk(struct gendisk *disk)
{
    struct my_device *dev = disk->private_data;

    /* Re-read capacity, etc. */
    set_capacity(disk, dev->new_size);

    return 0;
}

static const struct block_device_operations my_fops = {
    .owner           = THIS_MODULE,
    .check_events    = my_check_events,
    .revalidate_disk = my_revalidate_disk,
    /* ... */
};

/* Enable event polling */
disk->events = DISK_EVENT_MEDIA_CHANGE;
disk->event_flags = DISK_EVENT_FLAG_POLL;
```

## Queue Limits

Configure I/O constraints:

```c
static int my_init(void)
{
    /* ... allocate disk ... */

    struct queue_limits *lim = &disk->queue->limits;

    /* Logical block size (smallest addressable unit) */
    blk_queue_logical_block_size(disk->queue, 512);

    /* Physical block size (optimal I/O size) */
    blk_queue_physical_block_size(disk->queue, 4096);

    /* Maximum sectors per request */
    blk_queue_max_hw_sectors(disk->queue, 256);

    /* Discard (TRIM) support */
    blk_queue_max_discard_sectors(disk->queue, UINT_MAX);
    disk->queue->limits.discard_granularity = 512;

    /* ... */
}
```

## Handling Discard (TRIM)

Support discard requests for SSDs or sparse storage:

```c
static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx,
                                 const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct my_device *dev = rq->q->queuedata;

    blk_mq_start_request(rq);

    /* Check for discard request */
    if (req_op(rq) == REQ_OP_DISCARD) {
        sector_t sector = blk_rq_pos(rq);
        unsigned int len = blk_rq_bytes(rq);

        /* Zero the discarded region (or mark sparse) */
        memset(dev->data + (sector << SECTOR_SHIFT), 0, len);

        blk_mq_end_request(rq, BLK_STS_OK);
        return BLK_STS_OK;
    }

    /* Handle normal read/write */
    /* ... */
}
```

## Complete block_device_operations

```c
static const struct block_device_operations my_fops = {
    .owner           = THIS_MODULE,
    .open            = my_open,
    .release         = my_release,
    .ioctl           = my_ioctl,
    .getgeo          = my_getgeo,
    .check_events    = my_check_events,     /* Removable media */
    .revalidate_disk = my_revalidate_disk,  /* Media changed */
};
```

## Summary

| Operation | Purpose |
|-----------|---------|
| `open` | Device opened (check access, increment count) |
| `release` | Last user closed device |
| `ioctl` | Custom commands |
| `getgeo` | Report disk geometry |
| `check_events` | Poll for media changes |
| `revalidate_disk` | Update after media change |

## Further Reading

- [Block Device Operations](https://elixir.bootlin.com/linux/v6.6/source/include/linux/blkdev.h) - Structure definition
- [Block Layer](https://docs.kernel.org/block/index.html) - Kernel documentation
