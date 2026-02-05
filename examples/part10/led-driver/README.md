# LED Class Demo Driver

This example demonstrates how to implement LED class drivers with brightness control and hardware blink support.

## Features

- 3 virtual LEDs (red, green, blue)
- Brightness control (0-255)
- Hardware blink support via blink_set callback
- Default trigger assignment
- Named using standard LED naming convention

## Building

```bash
make
```

## Testing

```bash
# Load the module
sudo insmod led_demo.ko

# List the LEDs
ls /sys/class/leds/demo:*

# Control brightness
echo 128 | sudo tee /sys/class/leds/demo:red:status/brightness
echo 255 | sudo tee /sys/class/leds/demo:green:power/brightness
echo 0 | sudo tee /sys/class/leds/demo:blue:activity/brightness

# Read brightness
cat /sys/class/leds/demo:red:status/brightness

# View available triggers
cat /sys/class/leds/demo:red:status/trigger

# Set a trigger
echo heartbeat | sudo tee /sys/class/leds/demo:red:status/trigger
echo timer | sudo tee /sys/class/leds/demo:blue:activity/trigger

# Configure timer trigger
echo 100 | sudo tee /sys/class/leds/demo:blue:activity/delay_on
echo 900 | sudo tee /sys/class/leds/demo:blue:activity/delay_off

# Disable trigger
echo none | sudo tee /sys/class/leds/demo:red:status/trigger

# Check kernel log for LED operations
dmesg | grep "demo:" | tail -20

# Unload
sudo rmmod led_demo
```

## LED Naming Convention

LEDs follow the naming format: `devicename:color:function`

Examples:
- `demo:red:status` - Red status LED
- `demo:green:power` - Green power indicator
- `demo:blue:activity` - Blue activity LED

## sysfs Interface

```
/sys/class/leds/demo:red:status/
├── brightness      # Current brightness (0-max_brightness)
├── max_brightness  # Maximum brightness value (255)
├── trigger         # Current trigger [in brackets]
├── delay_on        # Blink on time (ms, when trigger=timer)
└── delay_off       # Blink off time (ms, when trigger=timer)
```

## Available Triggers

Common triggers include:
- `none` - No automatic control
- `default-on` - LED on by default
- `heartbeat` - Heartbeat pattern
- `timer` - Configurable on/off timing
- `disk-activity` - Disk I/O activity
- `cpu` - CPU activity
- `mmc0` - SD card activity

## Hardware Blink

The driver implements `blink_set` for hardware-accelerated blinking:

```c
static int demo_led_blink_set(struct led_classdev *cdev,
                              unsigned long *delay_on,
                              unsigned long *delay_off)
{
    /* Configure hardware blink */
    /* Return -EINVAL for software fallback */
    return 0;
}
```

If the hardware can't support the requested timing, return `-EINVAL` and the LED core will use software blinking.

## Key Implementation Details

### LED Registration

```c
led->cdev.name = "demo:red:status";
led->cdev.brightness_set = demo_led_brightness_set;
led->cdev.brightness_get = demo_led_brightness_get;
led->cdev.blink_set = demo_led_blink_set;
led->cdev.max_brightness = 255;
led->cdev.default_trigger = "heartbeat";

ret = devm_led_classdev_register(&pdev->dev, &led->cdev);
```

### Brightness Callback

```c
static void demo_led_brightness_set(struct led_classdev *cdev,
                                    enum led_brightness brightness)
{
    /* Apply brightness to hardware */
    /* brightness: 0 to cdev->max_brightness */
}
```

## Files

- `led_demo.c` - Main driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- Part 10.7: LED Subsystem
