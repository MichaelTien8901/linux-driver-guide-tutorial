# IIO ADC Demo Driver

This example demonstrates how to implement an IIO device driver for an ADC.

## Features

- 4-channel 12-bit ADC simulation
- Standard IIO sysfs interface
- Scale attribute for voltage calculation
- Writable raw values for testing

## Building

```bash
make
```

## Testing

```bash
# Load the module
sudo insmod iio_adc_demo.ko

# Find the IIO device
ls /sys/bus/iio/devices/

# Read raw values
cat /sys/bus/iio/devices/iio:device0/in_voltage0_raw
cat /sys/bus/iio/devices/iio:device0/in_voltage1_raw

# Read scale (for conversion to mV)
cat /sys/bus/iio/devices/iio:device0/in_voltage_scale

# Calculate voltage: raw * scale = mV
# Example: 2048 * 0.805664 = 1650 mV = 1.65V

# Set a test value (for testing)
echo 1000 | sudo tee /sys/bus/iio/devices/iio:device0/in_voltage0_raw

# Run test script
make test

# Unload
sudo rmmod iio_adc_demo
```

## sysfs Interface

```
/sys/bus/iio/devices/iio:deviceN/
├── name                    # "demo-iio-adc"
├── in_voltage0_raw         # Channel 0 raw value
├── in_voltage1_raw         # Channel 1 raw value
├── in_voltage2_raw         # Channel 2 raw value
├── in_voltage3_raw         # Channel 3 raw value
├── in_voltage_scale        # Scale factor (mV/LSB)
└── in_voltage_offset       # Offset (0)
```

## Voltage Calculation

```
Voltage (mV) = raw_value * scale
Voltage (V) = raw_value * scale / 1000

For 12-bit ADC with 3.3V reference:
scale = 3300 / 4096 = 0.805664 mV/LSB
```

## Key Implementation Details

### Channel Definition

```c
#define DEMO_ADC_CHANNEL(num) {
    .type = IIO_VOLTAGE,
    .indexed = 1,
    .channel = (num),
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
}
```

### Read Raw Callback

```c
static int demo_adc_read_raw(struct iio_dev *indio_dev,
                             struct iio_chan_spec const *chan,
                             int *val, int *val2, long mask)
{
    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        *val = read_adc_channel(chan->channel);
        return IIO_VAL_INT;

    case IIO_CHAN_INFO_SCALE:
        *val = VREF_MV;
        *val2 = ADC_RESOLUTION;
        return IIO_VAL_FRACTIONAL_LOG2;
    }
}
```

## Files

- `iio_adc_demo.c` - Main driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- Part 11.1: IIO Subsystem
- Part 11.2: IIO Driver
