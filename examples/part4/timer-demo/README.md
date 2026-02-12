# Timer Demo

Demonstrates kernel timer mechanisms: standard `timer_list` for jiffies-resolution polling and `hrtimer` for high-resolution timing.

## Files

- `timer_demo.c` - Kernel module with both timer types
- `Makefile` - Build configuration
- `README.md` - This file

## Features Demonstrated

1. **timer_list** - Standard kernel timer with jiffies resolution
   - `timer_setup()` initialization
   - `mod_timer()` for arming/re-arming
   - `del_timer_sync()` for safe cancellation
   - Periodic re-arming from callback

2. **hrtimer** - High-resolution timer with nanosecond precision
   - `hrtimer_init()` with CLOCK_MONOTONIC
   - `HRTIMER_RESTART` / `HRTIMER_NORESTART` return values
   - `hrtimer_cancel()` for cleanup

3. **Time conversions** - `msecs_to_jiffies()`, `HZ`, jiffies display

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
sudo insmod timer_demo.ko

# View status
cat /proc/timer_demo

# Start standard timer (1s interval)
echo "start" | sudo tee /proc/timer_demo

# Start high-resolution timer (500ms interval)
echo "hstart" | sudo tee /proc/timer_demo

# Wait and check counts
sleep 5
cat /proc/timer_demo

# Stop timers
echo "stop" | sudo tee /proc/timer_demo
echo "hstop" | sudo tee /proc/timer_demo

# Check kernel log
dmesg | tail -20

# Unload
sudo rmmod timer_demo
```

## Key Takeaways

- Timer callbacks run in softirq context — cannot sleep
- Use `del_timer_sync()` before freeing timer-containing structures
- `hrtimer` gives nanosecond resolution but with slightly more overhead
- Use `delayed_work` instead if your callback needs to sleep
