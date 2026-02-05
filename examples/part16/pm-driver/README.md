# Power Management Demo Driver

Demonstrates runtime PM and system sleep PM in a platform driver.

## What This Example Shows

- Runtime PM with `pm_runtime_get/put()`
- Autosuspend with configurable delay
- System suspend/resume callbacks
- sysfs attributes that trigger PM transitions
- Proper PM initialization sequence

## How It Works

The driver creates a sysfs attribute that requires the device to be "powered on" to access. When you read/write the attribute:

1. `pm_runtime_get_sync()` ensures device is active
2. Data is accessed
3. `pm_runtime_put_autosuspend()` marks for delayed suspend

After 2 seconds of inactivity, the device suspends.

## Building

```bash
make
```

## Testing

```bash
# Load module
sudo insmod pm_demo.ko

# Check initial state
cat /sys/devices/platform/pm_demo.0/status
cat /sys/devices/platform/pm_demo.0/power/runtime_status

# Read data (triggers runtime resume)
cat /sys/devices/platform/pm_demo.0/data

# Check status (should be "active")
cat /sys/devices/platform/pm_demo.0/power/runtime_status

# Wait for autosuspend (2 seconds)
sleep 3

# Check status (should be "suspended")
cat /sys/devices/platform/pm_demo.0/power/runtime_status

# Write data (triggers resume)
echo 100 | sudo tee /sys/devices/platform/pm_demo.0/data

# Watch kernel log
dmesg | grep pm_demo

# Run automated test
make test

# Unload
sudo rmmod pm_demo
```

## Testing System Sleep

```bash
# Load module
sudo insmod pm_demo.ko

# Suspend system (if supported)
sudo systemctl suspend

# After wake, check logs
dmesg | grep pm_demo
# Should show "System suspend" and "System resume" messages
```

## Key Code Sections

### PM Ops Definition

```c
static const struct dev_pm_ops pm_demo_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(pm_demo_suspend, pm_demo_resume)
    SET_RUNTIME_PM_OPS(pm_demo_runtime_suspend, pm_demo_runtime_resume, NULL)
};
```

### Runtime PM in Probe

```c
pm_runtime_set_autosuspend_delay(&pdev->dev, 2000);
pm_runtime_use_autosuspend(&pdev->dev);
pm_runtime_set_active(&pdev->dev);
pm_runtime_enable(&pdev->dev);
```

### Using Runtime PM

```c
static int pm_demo_hw_read(struct pm_demo_dev *pmdev)
{
    /* Ensure powered */
    pm_runtime_get_sync(pmdev->dev);

    /* Access hardware */
    data = read_register();

    /* Allow suspend after delay */
    pm_runtime_mark_last_busy(pmdev->dev);
    pm_runtime_put_autosuspend(pmdev->dev);

    return data;
}
```

## Files

- `pm_demo.c` - Complete driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- [Part 16.1: PM Concepts](../../../part16/01-concepts.md)
- [Part 16.2: PM Implementation](../../../part16/02-implementation.md)

## Further Reading

- [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)
- [Device PM](https://docs.kernel.org/driver-api/pm/devices.html)
