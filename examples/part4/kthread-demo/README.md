# Kthread Demo

Demonstrates kernel threads for background processing: a periodic polling thread and an event-driven thread using wait queues.

## Files

- `kthread_demo.c` - Kernel module with two kthread patterns
- `Makefile` - Build configuration
- `README.md` - This file

## Features Demonstrated

1. **Periodic polling kthread**
   - `kthread_run()` for creation and start
   - `wait_event_interruptible_timeout()` for timed sleeping
   - `kthread_should_stop()` for cooperative shutdown

2. **Event-driven kthread**
   - `wait_event_interruptible()` for sleeping until event
   - Combined stop check in wait condition
   - Wake-up from external trigger

3. **Clean shutdown**
   - `kthread_stop()` blocks until thread exits
   - Proper error handling in init with rollback

## Building

```bash
make
```

### Cross-compile for ARM64

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

## Testing

### Quick Test

```bash
make test
```

### Manual Testing

```bash
# Load module
sudo insmod kthread_demo.ko

# Wait for polling thread to accumulate some readings
sleep 5
cat /proc/kthread_demo

# Trigger events for the event thread
echo "event" | sudo tee /proc/kthread_demo
echo "event" | sudo tee /proc/kthread_demo

# Check results
cat /proc/kthread_demo

# Verify threads are running
ps -ef | grep kdemo

# Check kernel log
dmesg | tail -20

# Unload (threads stop cleanly)
sudo rmmod kthread_demo
```

## Key Takeaways

- `kthread_run()` creates and starts a thread in one call
- Always check `kthread_should_stop()` in the thread loop
- Include `kthread_should_stop()` in wait conditions so the thread wakes on stop
- `kthread_stop()` blocks — never call it on an already-exited thread
