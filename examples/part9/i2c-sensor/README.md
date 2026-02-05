# I2C Sensor Demo Driver

This example demonstrates how to write an I2C client driver using the regmap API and IIO subsystem.

## Features

- I2C client driver using `module_i2c_driver()`
- Regmap for register access with caching
- IIO subsystem integration for temperature and humidity
- Power management (suspend/resume)
- Device Tree compatible

## Prerequisites

```bash
# Ensure kernel headers are installed
sudo apt-get install linux-headers-$(uname -r)

# IIO tools for testing
sudo apt-get install iio-sensor-proxy
```

## Building

```bash
make
```

## Device Tree Binding

```dts
&i2c1 {
    status = "okay";

    demo_sensor: sensor@48 {
        compatible = "demo,i2c-sensor";
        reg = <0x48>;
    };
};
```

## Testing Without Hardware

Since this driver requires actual I2C hardware, you can test the build and module loading:

```bash
# Build the module
make

# Check module info
modinfo i2c_sensor_demo.ko

# The driver will fail to probe without hardware, but you can verify:
# - Module compiles correctly
# - Module loads into kernel (though no device will match)
sudo insmod i2c_sensor_demo.ko
dmesg | tail -5

# List I2C buses (if available)
ls /sys/bus/i2c/devices/

# Unload
sudo rmmod i2c_sensor_demo
```

## Using with Real Hardware

If you have an I2C sensor that matches the register map:

```bash
# Load the module
sudo insmod i2c_sensor_demo.ko

# Check for IIO device
ls /sys/bus/iio/devices/

# Read temperature (IIO interface)
cat /sys/bus/iio/devices/iio:device0/in_temp_raw

# Read humidity
cat /sys/bus/iio/devices/iio:device0/in_humidityrelative_raw

# Set calibration bias
echo 5 > /sys/bus/iio/devices/iio:device0/in_temp_calibbias
```

## Key Concepts Demonstrated

### Regmap Configuration

```c
static const struct regmap_config demo_sensor_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = REG_CALIB_HUMID,
    .cache_type = REGCACHE_RBTREE,
    .volatile_reg = demo_sensor_volatile,
    .readable_reg = demo_sensor_readable,
    .writeable_reg = demo_sensor_writeable,
};
```

### IIO Channel Definition

```c
static const struct iio_chan_spec demo_sensor_channels[] = {
    {
        .type = IIO_TEMP,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
                              BIT(IIO_CHAN_INFO_CALIBBIAS),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
    /* ... */
};
```

### Power Management

```c
static int demo_sensor_suspend(struct device *dev)
{
    regmap_write(sensor->regmap, REG_CONFIG, 0);
    regmap_cache_mark_dirty(sensor->regmap);
    return 0;
}

static int demo_sensor_resume(struct device *dev)
{
    regmap_cache_sync(sensor->regmap);
    return 0;
}
```

## Files

- `i2c_sensor_demo.c` - Main driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- Part 9.1: I2C Subsystem
- Part 9.2: I2C Client Driver
- Part 9.3: I2C with Regmap
