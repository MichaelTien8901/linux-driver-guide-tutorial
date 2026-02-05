# HWMON Demo Driver

This example demonstrates how to implement a hardware monitoring driver using the hwmon_chip_info structure.

## Features

- 2 temperature sensors with labels, max, and critical thresholds
- 2 voltage sensors with labels, min, and max thresholds
- 1 fan speed sensor with label and minimum threshold
- Alarm status for threshold violations
- Simulated values with realistic variation

## Building

```bash
make
```

## Testing

```bash
# Load the module
sudo insmod hwmon_demo.ko

# Run the test script
make test

# Or manually read sensors
HWMON=/sys/class/hwmon/hwmon*  # Find the correct one

# Read temperatures (in millidegrees)
cat $HWMON/temp1_input   # CPU temperature
cat $HWMON/temp1_label   # "CPU"
cat $HWMON/temp2_input   # Ambient temperature
cat $HWMON/temp2_label   # "Ambient"

# Read voltages (in millivolts)
cat $HWMON/in0_input     # 3.3V rail
cat $HWMON/in1_input     # 5V rail

# Read fan speed (in RPM)
cat $HWMON/fan1_input

# Set thresholds
echo 75000 | sudo tee $HWMON/temp1_max  # Set max to 75°C

# Check alarms
cat $HWMON/temp1_max_alarm
cat $HWMON/fan1_alarm

# Use lm-sensors
sensors demo_hwmon-*
```

## sysfs Attributes

```
/sys/class/hwmon/hwmonN/
├── name                # "demo_hwmon"
├── temp1_input         # Temperature 1 (millidegrees)
├── temp1_max           # Max threshold
├── temp1_crit          # Critical threshold
├── temp1_label         # "CPU"
├── temp1_max_alarm     # 1 if temp > max
├── temp1_crit_alarm    # 1 if temp > crit
├── temp2_input         # Temperature 2
├── temp2_label         # "Ambient"
├── in0_input           # Voltage 0 (millivolts)
├── in0_min             # Min threshold
├── in0_max             # Max threshold
├── in0_label           # "3.3V Rail"
├── in1_input           # Voltage 1
├── in1_label           # "5V Rail"
├── fan1_input          # Fan speed (RPM)
├── fan1_min            # Min threshold
├── fan1_label          # "System Fan"
└── fan1_alarm          # 1 if RPM < min
```

## Unit Conventions

| Sensor Type | Unit | Scale |
|-------------|------|-------|
| Temperature | °C | millidegrees (1000 = 1°C) |
| Voltage | V | millivolts (1000 = 1V) |
| Fan Speed | RPM | 1 RPM |

## Key Implementation Details

### Channel Configuration

```c
static const struct hwmon_channel_info * const demo_hwmon_info[] = {
    HWMON_CHANNEL_INFO(temp,
        HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT | HWMON_T_LABEL,
        HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT | HWMON_T_LABEL),
    HWMON_CHANNEL_INFO(in,
        HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX | HWMON_I_LABEL,
        HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX | HWMON_I_LABEL),
    HWMON_CHANNEL_INFO(fan,
        HWMON_F_INPUT | HWMON_F_MIN | HWMON_F_LABEL | HWMON_F_ALARM),
    NULL
};
```

### Read Callback

```c
static int demo_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
                           u32 attr, int channel, long *val)
{
    switch (type) {
    case hwmon_temp:
        if (attr == hwmon_temp_input) {
            *val = temp_millidegrees;
            return 0;
        }
        break;
    /* ... */
    }
    return -EOPNOTSUPP;
}
```

## Files

- `hwmon_demo.c` - Main driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- Part 10.5: HWMON Subsystem
- Part 10.6: HWMON Driver
