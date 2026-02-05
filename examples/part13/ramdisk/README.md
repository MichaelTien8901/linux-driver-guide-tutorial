# RAM Disk Demo

A simple RAM-backed block device using the blk-mq interface.

## What This Example Shows

- blk-mq setup with `blk_mq_tag_set`
- Disk allocation with `blk_mq_alloc_disk()`
- Request processing in `queue_rq` callback
- Segment iteration with `rq_for_each_segment`
- Proper cleanup sequence

## How It Works

The driver allocates 16MB of kernel memory and exposes it as a block device. Any data written to `/dev/ramdemo0` is stored in RAM and lost when the module unloads.

## Building

```bash
make
```

## Testing

```bash
# Load module
sudo insmod ramdisk_demo.ko

# Verify device exists
ls -la /dev/ramdemo0
lsblk /dev/ramdemo0

# Create filesystem and mount
sudo mkfs.ext4 /dev/ramdemo0
sudo mkdir -p /mnt/ramdemo
sudo mount /dev/ramdemo0 /mnt/ramdemo

# Use it like any disk
echo "Hello RAM disk!" | sudo tee /mnt/ramdemo/test.txt
cat /mnt/ramdemo/test.txt
df -h /mnt/ramdemo

# Cleanup
sudo umount /mnt/ramdemo
sudo rmmod ramdisk_demo

# Or run automated test
make test
```

## Key Code Sections

### blk-mq Setup

```c
/* Tag set configuration */
tag_set.ops = &ramdisk_mq_ops;
tag_set.nr_hw_queues = 1;
tag_set.queue_depth = 128;
blk_mq_alloc_tag_set(&tag_set);

/* Allocate disk with queue */
disk = blk_mq_alloc_disk(&tag_set, driver_data);
set_capacity(disk, size_in_sectors);
add_disk(disk);
```

### Request Processing

```c
static blk_status_t ramdisk_queue_rq(struct blk_mq_hw_ctx *hctx,
                                      const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    blk_mq_start_request(rq);

    rq_for_each_segment(bvec, rq, iter) {
        void *buf = kmap_local_page(bvec.bv_page) + bvec.bv_offset;

        if (rq_data_dir(rq) == WRITE)
            memcpy(device_mem + pos, buf, bvec.bv_len);
        else
            memcpy(buf, device_mem + pos, bvec.bv_len);

        kunmap_local(buf);
    }

    blk_mq_end_request(rq, BLK_STS_OK);
    return BLK_STS_OK;
}
```

## Files

- `ramdisk_demo.c` - Complete driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- [Part 13.1: Block Driver Concepts](../../../part13/01-concepts.md)
- [Part 13.2: Driver Skeleton](../../../part13/02-driver-skeleton.md)

## Further Reading

- [blk-mq Documentation](https://docs.kernel.org/block/blk-mq.html)
- [Block Layer](https://docs.kernel.org/block/index.html)
