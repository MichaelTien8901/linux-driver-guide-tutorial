# Spinlock Demonstration

Demonstrates various spinlock patterns and alternatives.

## Files

- `spinlock_demo.c` - Kernel module with spinlock examples
- `Makefile` - Build configuration

## Features Demonstrated

1. **Basic Spinlock**
   - `spin_lock()` / `spin_unlock()`
   - Protecting a simple counter

2. **Reader-Writer Spinlock**
   - `read_lock()` / `read_unlock()`
   - `write_lock()` / `write_unlock()`
   - Multiple readers, exclusive writers

3. **Per-CPU Data**
   - `DEFINE_PER_CPU()`
   - `this_cpu_inc()`
   - Lock-free per-CPU access

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
sudo insmod spinlock_demo.ko

# View stats
cat /proc/spinlock_demo

# Sample output:
# Basic counter: 12345
# RW data:
#   value: 456
#   name: update_456
#   reads: 98765
#   writes: 456
# Per-CPU counters:
#   CPU0: 3456
#   CPU1: 3421
#   Total: 6877

# Watch stats update in real-time
watch -n 1 cat /proc/spinlock_demo

# Unload module
sudo rmmod spinlock_demo
```

### Check Kernel Log

```bash
dmesg | tail -20
# Shows thread start/stop messages
```

## Code Explanation

### Basic Spinlock

```c
struct basic_counter {
    spinlock_t lock;
    unsigned long count;
};

spin_lock(&basic.lock);
basic.count++;
spin_unlock(&basic.lock);
```

### RW Spinlock

```c
/* Reader - multiple can run concurrently */
read_lock(&rwdata.lock);
value = rwdata.value;
read_unlock(&rwdata.lock);

/* Writer - exclusive access */
write_lock(&rwdata.lock);
rwdata.value++;
write_unlock(&rwdata.lock);
```

### Per-CPU Data

```c
DEFINE_PER_CPU(unsigned long, percpu_counter);

/* No lock needed! */
preempt_disable();
this_cpu_inc(percpu_counter);
preempt_enable();
```

## Performance Comparison

In this demo:

| Pattern | Contention | Overhead |
|---------|------------|----------|
| Basic spinlock | High | Medium |
| RW spinlock | Lower (more readers) | Higher |
| Per-CPU | None | Lowest |

## Key Takeaways

1. **Use spinlocks for short critical sections**
2. **Use RW locks when reads >> writes**
3. **Consider per-CPU data to avoid locking entirely**
4. **Never sleep while holding a spinlock**
