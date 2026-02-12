# Input Device Demo

Demonstrates the Linux input subsystem by creating a virtual button device. Events are triggered by writing commands to a proc interface and can be observed with `evtest`.

## Files

- `input_demo.c` - Virtual input device with proc control
- `test_input.sh` - Automated test script
- `Makefile` - Build configuration
- `README.md` - This file

## Features Demonstrated

1. **Input device registration**
   - `input_allocate_device()` and `input_register_device()`
   - Setting device name, physical path, bus type

2. **Capability declaration**
   - `input_set_capability()` for EV_KEY events
   - KEY_ENTER and KEY_POWER key codes

3. **Event reporting**
   - `input_report_key()` for press/release
   - `input_sync()` to mark frame boundary

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
sudo insmod input_demo.ko

# Find the event device
cat /proc/bus/input/devices | grep -A 4 "Virtual Button"

# In one terminal, monitor events (install evtest if needed)
evtest /dev/input/eventN  # Replace N with your device number

# In another terminal, trigger events
echo "press" | sudo tee /proc/input_demo
echo "release" | sudo tee /proc/input_demo
echo "click" | sudo tee /proc/input_demo
echo "power" | sudo tee /proc/input_demo

# View statistics
cat /proc/input_demo

# Unload
sudo rmmod input_demo
```

### Expected evtest Output

```
Event: time ..., type 1 (EV_KEY), code 28 (KEY_ENTER), value 1
Event: time ..., -------------- SYN_REPORT ------------
Event: time ..., type 1 (EV_KEY), code 28 (KEY_ENTER), value 0
Event: time ..., -------------- SYN_REPORT ------------
```

## Key Takeaways

- Always call `input_sync()` after reporting events
- Use `input_set_capability()` to declare supported events
- Virtual devices use `BUS_VIRTUAL` bus type
- `evtest` is the standard tool for debugging input devices
