---
layout: default
title: "13.4 Request Handling"
parent: "Part 13: Block Device Drivers"
nav_order: 4
---

# Request Handling

This chapter covers advanced request processing: multiple queues, error handling, and request types.

## Request Types

Check what operation is requested:

```c
static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx,
                                 const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;

    blk_mq_start_request(rq);

    switch (req_op(rq)) {
    case REQ_OP_READ:
        return do_read(rq);

    case REQ_OP_WRITE:
        return do_write(rq);

    case REQ_OP_FLUSH:
        return do_flush(rq);

    case REQ_OP_DISCARD:
        return do_discard(rq);

    case REQ_OP_WRITE_ZEROES:
        return do_write_zeroes(rq);

    default:
        blk_mq_end_request(rq, BLK_STS_NOTSUPP);
        return BLK_STS_OK;
    }
}
```

| Operation | Meaning |
|-----------|---------|
| `REQ_OP_READ` | Read data |
| `REQ_OP_WRITE` | Write data |
| `REQ_OP_FLUSH` | Flush volatile cache to stable storage |
| `REQ_OP_DISCARD` | Discard sectors (TRIM for SSDs) |
| `REQ_OP_WRITE_ZEROES` | Write zeros efficiently |

## Error Handling

Return appropriate status codes:

```c
static blk_status_t do_read(struct request *rq)
{
    struct my_device *dev = rq->q->queuedata;
    sector_t sector = blk_rq_pos(rq);

    /* Check bounds */
    if (sector + blk_rq_sectors(rq) > get_capacity(rq->q->disk)) {
        blk_mq_end_request(rq, BLK_STS_IOERR);
        return BLK_STS_OK;
    }

    /* Check device state */
    if (!dev->online) {
        blk_mq_end_request(rq, BLK_STS_OFFLINE);
        return BLK_STS_OK;
    }

    /* Simulate hardware error */
    if (dev->error_injection) {
        blk_mq_end_request(rq, BLK_STS_MEDIUM);
        return BLK_STS_OK;
    }

    /* Process normally... */
    return BLK_STS_OK;
}
```

| Status | Meaning |
|--------|---------|
| `BLK_STS_OK` | Success |
| `BLK_STS_IOERR` | Generic I/O error |
| `BLK_STS_MEDIUM` | Medium error (bad sector) |
| `BLK_STS_TIMEOUT` | Timeout |
| `BLK_STS_RESOURCE` | Temporary resource shortage, retry |
| `BLK_STS_NOTSUPP` | Operation not supported |
| `BLK_STS_OFFLINE` | Device offline |

## Returning BLK_STS_RESOURCE

When your driver is temporarily busy:

```c
static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx,
                                 const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct my_device *dev = rq->q->queuedata;

    /* Check if we can accept this request */
    if (atomic_read(&dev->pending_requests) >= MAX_PENDING) {
        /* Don't call blk_mq_start_request() */
        return BLK_STS_RESOURCE;  /* Will be retried */
    }

    blk_mq_start_request(rq);
    /* ... process request ... */
    return BLK_STS_OK;
}
```

{: .warning }
> Don't call `blk_mq_start_request()` before returning `BLK_STS_RESOURCE`. The block layer will re-queue and retry.

## Multiple Hardware Queues

For devices with multiple submission queues:

```c
struct my_device {
    struct blk_mq_tag_set tag_set;
    struct my_hw_queue *hw_queues;
    int nr_hw_queues;
};

static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx,
                                 const struct blk_mq_queue_data *bd)
{
    /* hctx->queue_num identifies which hardware queue */
    struct my_hw_queue *hwq = hctx->driver_data;

    /* Submit to specific hardware queue */
    return submit_to_hwq(hwq, bd->rq);
}

static int my_init_hctx(struct blk_mq_hw_ctx *hctx, void *data,
                        unsigned int hctx_idx)
{
    struct my_device *dev = data;

    /* Associate hardware queue with this hctx */
    hctx->driver_data = &dev->hw_queues[hctx_idx];
    return 0;
}

static const struct blk_mq_ops my_mq_ops = {
    .queue_rq = my_queue_rq,
    .init_hctx = my_init_hctx,
};

/* In init */
tag_set.nr_hw_queues = num_cpus_or_queues;
```

## Asynchronous Completion

For hardware that signals completion via interrupt:

```c
struct my_cmd {
    struct request *rq;
    /* Hardware command data */
};

static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hctx,
                                 const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct my_cmd *cmd = blk_mq_rq_to_pdu(rq);

    cmd->rq = rq;

    blk_mq_start_request(rq);

    /* Submit to hardware - completion comes via interrupt */
    submit_cmd_to_hw(cmd);

    return BLK_STS_OK;  /* Don't end request here */
}

/* Called from interrupt handler or completion tasklet */
static void my_complete_cmd(struct my_cmd *cmd, int status)
{
    blk_status_t blk_status = status ? BLK_STS_IOERR : BLK_STS_OK;

    blk_mq_end_request(cmd->rq, blk_status);
}
```

Configure `cmd_size` to allocate space for your command structure:

```c
tag_set.cmd_size = sizeof(struct my_cmd);

/* In queue_rq, get the command structure */
struct my_cmd *cmd = blk_mq_rq_to_pdu(rq);
```

## Timeout Handling

Handle requests that take too long:

```c
static enum blk_eh_timer_return my_timeout(struct request *rq)
{
    struct my_cmd *cmd = blk_mq_rq_to_pdu(rq);

    /* Try to recover */
    if (abort_hw_cmd(cmd)) {
        /* Hardware aborted - we'll complete it */
        blk_mq_end_request(rq, BLK_STS_TIMEOUT);
        return BLK_EH_DONE;
    }

    /* Couldn't abort - reset needed */
    return BLK_EH_RESET_TIMER;  /* Retry timeout */
}

static const struct blk_mq_ops my_mq_ops = {
    .queue_rq = my_queue_rq,
    .timeout = my_timeout,
};

/* Set timeout value */
blk_queue_rq_timeout(disk->queue, 30 * HZ);  /* 30 seconds */
```

## Flush Support

For devices with volatile write cache:

```c
static blk_status_t do_flush(struct request *rq)
{
    struct my_device *dev = rq->q->queuedata;

    /* Ensure all writes are committed to stable storage */
    sync_to_backing_store(dev);

    blk_mq_end_request(rq, BLK_STS_OK);
    return BLK_STS_OK;
}

/* Enable flush support */
blk_queue_write_cache(disk->queue, true, true);
/* Parameters: write_cache, fua (Force Unit Access) */
```

## Request Helpers

Useful functions for processing requests:

```c
/* Request position and size */
sector_t pos = blk_rq_pos(rq);           /* Starting sector */
unsigned int sectors = blk_rq_sectors(rq); /* Number of sectors */
unsigned int bytes = blk_rq_bytes(rq);   /* Total bytes */

/* Direction */
bool is_write = rq_data_dir(rq);         /* 0=read, 1=write */
enum req_op op = req_op(rq);             /* Full operation type */

/* Partial completion (for multi-segment) */
blk_mq_end_request_batch(rq, BLK_STS_OK);

/* Update bytes completed without ending request */
blk_mq_update_nr_bytes(rq, bytes_done);
```

## Summary

| Concept | Purpose |
|---------|---------|
| `req_op(rq)` | Get operation type |
| `blk_mq_start_request()` | Mark request as started |
| `blk_mq_end_request()` | Complete with status |
| `BLK_STS_RESOURCE` | Temporary busy, will retry |
| `blk_mq_rq_to_pdu()` | Get per-request private data |
| `.timeout` | Handle stuck requests |

## Further Reading

- [blk-mq Documentation](https://docs.kernel.org/block/blk-mq.html) - Multi-queue interface
- [Block Layer](https://docs.kernel.org/block/index.html) - Complete documentation
