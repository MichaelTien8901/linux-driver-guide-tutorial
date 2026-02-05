# Threaded IRQ Demo

Demonstrates threaded interrupt handling patterns from Part 7.

## Features

- **Hardirq Handler**: Fast top-half that checks and acknowledges interrupts
- **Threaded Handler**: Bottom-half in process context that can sleep
- **Mutex Usage**: Shows safe locking in threaded handler
- **Wait Queue**: Demonstrates waking up user space on data arrival
- **Statistics**: Tracks interrupt counts and latencies

## Architecture

```
Timer (simulates hardware)
         |
         v
    [Hardirq Handler]     <- Runs in interrupt context
    - Check pending flag     Cannot sleep!
    - Acknowledge IRQ
    - Return IRQ_WAKE_THREAD
         |
         v
    [Threaded Handler]    <- Runs in process context
    - Lock mutex             CAN sleep!
    - Process data
    - Wake waiters
    - Return IRQ_HANDLED
```

## Building

```bash
make
```

## Usage

```bash
# Load module
sudo insmod threaded_irq_demo.ko

# Start generating simulated interrupts
echo start | sudo tee /proc/threaded_irq_demo/control

# View statistics
cat /proc/threaded_irq_demo/stats

# View data buffer
cat /proc/threaded_irq_demo/data

# Change interrupt interval (milliseconds)
echo "interval 50" | sudo tee /proc/threaded_irq_demo/control

# Stop interrupts
echo stop | sudo tee /proc/threaded_irq_demo/control

# Reset statistics
echo reset | sudo tee /proc/threaded_irq_demo/control

# Unload module
sudo rmmod threaded_irq_demo
```

## Quick Test

```bash
make test
```

## Proc Interface

| File | Access | Description |
|------|--------|-------------|
| `/proc/threaded_irq_demo/control` | RW | Control commands |
| `/proc/threaded_irq_demo/stats` | RO | Interrupt statistics |
| `/proc/threaded_irq_demo/data` | RO | Data buffer contents |

### Control Commands

| Command | Description |
|---------|-------------|
| `start` | Start generating interrupts |
| `stop` | Stop generating interrupts |
| `reset` | Reset all statistics |
| `interval <ms>` | Set interrupt interval |

## Statistics Explained

| Statistic | Meaning |
|-----------|---------|
| Hardirq count | Number of times hardirq handler ran |
| Thread count | Number of times threaded handler ran |
| Spurious count | Interrupts without pending flag (shouldn't happen) |
| Avg latency | Average time from hardirq to thread completion |
| Data in buffer | Number of entries in circular buffer |

## Code Walkthrough

### Hardirq Handler (Top Half)

```c
static irqreturn_t demo_hardirq(int irq, void *dev_id)
{
    struct irq_demo_device *dev = dev_id;

    /* Check if this interrupt is ours */
    if (!atomic_read(&dev->pending_irq))
        return IRQ_NONE;  /* Not our interrupt */

    /* Acknowledge the interrupt */
    atomic_set(&dev->pending_irq, 0);
    atomic_inc(&dev->hardirq_count);

    /* Wake the threaded handler */
    return IRQ_WAKE_THREAD;
}
```

### Threaded Handler (Bottom Half)

```c
static irqreturn_t demo_thread_handler(int irq, void *dev_id)
{
    struct irq_demo_device *dev = dev_id;

    /* CAN use mutex because we're in process context! */
    mutex_lock(&dev->data_mutex);

    /* Process data into buffer */
    dev->data_buffer[dev->data_head] = dev->hw_data;
    dev->data_head = (dev->data_head + 1) % BUFFER_SIZE;

    mutex_unlock(&dev->data_mutex);

    /* Wake up readers */
    wake_up_interruptible(&dev->data_ready);

    return IRQ_HANDLED;
}
```

### Registration (In Real Driver)

```c
/* Real driver would use: */
ret = devm_request_threaded_irq(&pdev->dev, irq,
                                demo_hardirq,
                                demo_thread_handler,
                                IRQF_ONESHOT,
                                "demo", dev);
```

## Key Takeaways

1. **Hardirq cannot sleep** - Use atomic operations, spinlocks only
2. **Threaded handler can sleep** - Mutex, GFP_KERNEL, etc. are OK
3. **Use IRQF_ONESHOT** - Keeps IRQ disabled until thread completes
4. **Return IRQ_NONE** - If interrupt isn't from your device
5. **Return IRQ_WAKE_THREAD** - To invoke threaded handler

## Simulation Note

This demo uses a kernel timer to simulate hardware interrupts. In a real driver:

```c
/* Real hardware IRQ request */
irq = platform_get_irq(pdev, 0);
ret = devm_request_threaded_irq(&pdev->dev, irq,
                                my_hardirq, my_thread,
                                IRQF_ONESHOT, "mydev", dev);
```

The handler functions are the same - only the interrupt source differs.

## References

- [Part 7.3: Interrupt Handlers](../../../part7/03-interrupt-handlers.md)
- [Part 7.4: Top and Bottom Halves](../../../part7/04-top-bottom-halves.md)
- [Part 7.6: Threaded IRQs](../../../part7/06-threaded-irqs.md)
