# Work Queue Demonstration

Demonstrates work queue usage patterns for deferred processing.

## Files

- `workqueue_demo.c` - Kernel module demonstrating work queues
- `Makefile` - Build configuration

## Features Demonstrated

1. **System Work Queue**
   - `schedule_work()` - queue on system workqueue
   - Simple, immediate execution

2. **Delayed Work**
   - `schedule_delayed_work()` - execute after delay
   - Useful for timeouts and delayed processing

3. **Custom Work Queue**
   - `alloc_workqueue()` - dedicated work queue
   - Custom flags and worker limits

4. **Periodic Work**
   - Self-rescheduling delayed work
   - Common pattern for polling/monitoring

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
sudo insmod workqueue_demo.ko

# View stats and commands
cat /proc/workqueue_demo

# Queue immediate work
echo "immediate" | sudo tee /proc/workqueue_demo

# Queue delayed work (2 second delay)
echo "delayed" | sudo tee /proc/workqueue_demo

# Queue custom work (to dedicated queue)
echo "custom" | sudo tee /proc/workqueue_demo

# Start periodic work (every 1 second)
echo "start" | sudo tee /proc/workqueue_demo

# Stop periodic work
echo "stop" | sudo tee /proc/workqueue_demo

# Watch stats update
watch -n 1 cat /proc/workqueue_demo

# Check kernel log for messages
dmesg | tail -30

# Unload
sudo rmmod workqueue_demo
```

## Code Patterns

### System Work Queue

```c
static struct work_struct my_work;

static void my_handler(struct work_struct *work)
{
    /* Can sleep here! */
    msleep(100);
}

INIT_WORK(&my_work, my_handler);
schedule_work(&my_work);
```

### Delayed Work

```c
static struct delayed_work my_delayed;

static void delayed_handler(struct work_struct *work)
{
    pr_info("Delayed work executing\n");
}

INIT_DELAYED_WORK(&my_delayed, delayed_handler);
schedule_delayed_work(&my_delayed, 2 * HZ);  /* 2 seconds */
```

### Custom Work Queue

```c
static struct workqueue_struct *my_wq;

my_wq = alloc_workqueue("my_wq", WQ_UNBOUND | WQ_MEM_RECLAIM, 4);
queue_work(my_wq, &work);

/* Cleanup */
flush_workqueue(my_wq);
destroy_workqueue(my_wq);
```

### Periodic Work

```c
static struct delayed_work periodic;
static bool running;

static void periodic_handler(struct work_struct *work)
{
    /* Do periodic task */

    /* Reschedule */
    if (running)
        schedule_delayed_work(&periodic, HZ);
}

/* Start */
running = true;
schedule_delayed_work(&periodic, HZ);

/* Stop */
running = false;
cancel_delayed_work_sync(&periodic);
```

### Work with Data

```c
struct my_work {
    struct work_struct work;
    int data;
};

static void handler(struct work_struct *work)
{
    struct my_work *mw = container_of(work, struct my_work, work);
    pr_info("Data: %d\n", mw->data);
    kfree(mw);
}

/* Allocate and queue */
struct my_work *mw = kmalloc(sizeof(*mw), GFP_KERNEL);
mw->data = 42;
INIT_WORK(&mw->work, handler);
schedule_work(&mw->work);
```

## Work Queue Flags

| Flag | Description |
|------|-------------|
| `WQ_UNBOUND` | Not bound to specific CPU |
| `WQ_HIGHPRI` | High priority workers |
| `WQ_MEM_RECLAIM` | Safe for memory reclaim |
| `WQ_CPU_INTENSIVE` | May run long, don't block others |
| `WQ_FREEZABLE` | Participates in system freeze |

## Key Takeaways

1. **Work queues run in process context** - can sleep
2. **Use system workqueue for simple cases**
3. **Create custom queue for high-priority or isolated work**
4. **Always cancel work before freeing resources**
5. **Use `_sync` variants to wait for completion**
