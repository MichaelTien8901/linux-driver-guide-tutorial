// SPDX-License-Identifier: GPL-2.0
/*
 * RAM Disk Demo Driver
 *
 * A simple RAM-backed block device demonstrating:
 * - blk-mq setup (tag_set, disk allocation)
 * - queue_rq callback for request processing
 * - Segment iteration with rq_for_each_segment
 *
 * Creates /dev/ramdemo0 - a 16MB RAM disk
 */

#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/vmalloc.h>

#define DRIVER_NAME "ramdisk_demo"
#define DISK_SIZE_MB 16
#define DISK_SIZE (DISK_SIZE_MB * 1024 * 1024)
#define SECTOR_SIZE 512

struct ramdisk_dev {
    struct gendisk *disk;
    struct blk_mq_tag_set tag_set;
    void *data;
    size_t size;
};

static struct ramdisk_dev ramdisk;

/* ============ Request Processing ============ */

static blk_status_t ramdisk_queue_rq(struct blk_mq_hw_ctx *hctx,
                                      const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct ramdisk_dev *dev = rq->q->queuedata;
    struct req_iterator iter;
    struct bio_vec bvec;
    loff_t pos = blk_rq_pos(rq) << SECTOR_SHIFT;
    loff_t dev_size = dev->size;

    /* Must call before processing */
    blk_mq_start_request(rq);

    /* Iterate over all segments in the request */
    rq_for_each_segment(bvec, rq, iter) {
        unsigned int len = bvec.bv_len;
        void *buf;

        /* Bounds check */
        if (pos + len > dev_size) {
            pr_err("I/O beyond device size: pos=%lld len=%u size=%lld\n",
                   pos, len, dev_size);
            blk_mq_end_request(rq, BLK_STS_IOERR);
            return BLK_STS_IOERR;
        }

        /* Map page to kernel address */
        buf = kmap_local_page(bvec.bv_page) + bvec.bv_offset;

        if (rq_data_dir(rq) == WRITE)
            memcpy(dev->data + pos, buf, len);
        else
            memcpy(buf, dev->data + pos, len);

        kunmap_local(buf);
        pos += len;
    }

    blk_mq_end_request(rq, BLK_STS_OK);
    return BLK_STS_OK;
}

static const struct blk_mq_ops ramdisk_mq_ops = {
    .queue_rq = ramdisk_queue_rq,
};

/* ============ Block Device Operations ============ */

static int ramdisk_open(struct gendisk *disk, blk_mode_t mode)
{
    pr_info("%s: opened\n", disk->disk_name);
    return 0;
}

static void ramdisk_release(struct gendisk *disk)
{
    pr_info("%s: released\n", disk->disk_name);
}

static const struct block_device_operations ramdisk_fops = {
    .owner = THIS_MODULE,
    .open = ramdisk_open,
    .release = ramdisk_release,
};

/* ============ Module Init/Exit ============ */

static int __init ramdisk_init(void)
{
    int err;

    /* Allocate backing memory */
    ramdisk.size = DISK_SIZE;
    ramdisk.data = vzalloc(ramdisk.size);
    if (!ramdisk.data) {
        pr_err("Failed to allocate %zu bytes\n", ramdisk.size);
        return -ENOMEM;
    }

    /* Set up tag set for blk-mq */
    memset(&ramdisk.tag_set, 0, sizeof(ramdisk.tag_set));
    ramdisk.tag_set.ops = &ramdisk_mq_ops;
    ramdisk.tag_set.nr_hw_queues = 1;
    ramdisk.tag_set.queue_depth = 128;
    ramdisk.tag_set.numa_node = NUMA_NO_NODE;
    ramdisk.tag_set.flags = BLK_MQ_F_SHOULD_MERGE;

    err = blk_mq_alloc_tag_set(&ramdisk.tag_set);
    if (err) {
        pr_err("Failed to allocate tag set: %d\n", err);
        goto free_data;
    }

    /* Allocate gendisk and request queue */
    ramdisk.disk = blk_mq_alloc_disk(&ramdisk.tag_set, &ramdisk);
    if (IS_ERR(ramdisk.disk)) {
        err = PTR_ERR(ramdisk.disk);
        pr_err("Failed to allocate disk: %d\n", err);
        goto free_tag_set;
    }

    /* Configure the disk */
    ramdisk.disk->major = 0;  /* Dynamically allocate major */
    ramdisk.disk->first_minor = 0;
    ramdisk.disk->minors = 1;  /* No partitions */
    ramdisk.disk->fops = &ramdisk_fops;
    snprintf(ramdisk.disk->disk_name, DISK_NAME_LEN, "ramdemo0");
    set_capacity(ramdisk.disk, ramdisk.size / SECTOR_SIZE);

    /* Store our device for queue_rq */
    ramdisk.disk->queue->queuedata = &ramdisk;

    /* Set physical/logical block size */
    blk_queue_physical_block_size(ramdisk.disk->queue, SECTOR_SIZE);
    blk_queue_logical_block_size(ramdisk.disk->queue, SECTOR_SIZE);

    /* Register the disk */
    err = add_disk(ramdisk.disk);
    if (err) {
        pr_err("Failed to add disk: %d\n", err);
        goto cleanup_disk;
    }

    pr_info("RAM disk registered: /dev/%s (%d MB)\n",
            ramdisk.disk->disk_name, DISK_SIZE_MB);
    return 0;

cleanup_disk:
    put_disk(ramdisk.disk);
free_tag_set:
    blk_mq_free_tag_set(&ramdisk.tag_set);
free_data:
    vfree(ramdisk.data);
    return err;
}

static void __exit ramdisk_exit(void)
{
    del_gendisk(ramdisk.disk);
    put_disk(ramdisk.disk);
    blk_mq_free_tag_set(&ramdisk.tag_set);
    vfree(ramdisk.data);
    pr_info("RAM disk unregistered\n");
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("RAM Disk Demo using blk-mq");
