---
layout: default
title: "11.2 IIO Driver"
parent: "Part 11: IIO, RTC, Regulator, Clock Drivers"
nav_order: 2
---

# IIO Driver

This chapter covers implementing IIO device drivers for sensors and data acquisition devices.

## Basic IIO Driver Structure

```c
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

struct my_sensor {
    struct regmap *regmap;
    struct mutex lock;
};

static int my_sensor_read_raw(struct iio_dev *indio_dev,
                              struct iio_chan_spec const *chan,
                              int *val, int *val2, long mask);

static int my_sensor_write_raw(struct iio_dev *indio_dev,
                               struct iio_chan_spec const *chan,
                               int val, int val2, long mask);

static const struct iio_info my_sensor_info = {
    .read_raw = my_sensor_read_raw,
    .write_raw = my_sensor_write_raw,
};
```

## Channel Definition

### Single-Ended ADC Channels

```c
#define MY_ADC_CHANNEL(num) {                          \
    .type = IIO_VOLTAGE,                               \
    .indexed = 1,                                      \
    .channel = (num),                                  \
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),     \
    .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE), \
    .scan_index = (num),                              \
    .scan_type = {                                    \
        .sign = 'u',                                  \
        .realbits = 12,                               \
        .storagebits = 16,                            \
        .shift = 0,                                   \
        .endianness = IIO_BE,                         \
    },                                                \
}

static const struct iio_chan_spec my_adc_channels[] = {
    MY_ADC_CHANNEL(0),
    MY_ADC_CHANNEL(1),
    MY_ADC_CHANNEL(2),
    MY_ADC_CHANNEL(3),
    IIO_CHAN_SOFT_TIMESTAMP(4),  /* Timestamp channel */
};
```

### Accelerometer Channels

```c
#define MY_ACCEL_CHANNEL(axis, idx) {                  \
    .type = IIO_ACCEL,                                 \
    .modified = 1,                                     \
    .channel2 = IIO_MOD_##axis,                        \
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |    \
                          BIT(IIO_CHAN_INFO_CALIBBIAS),\
    .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) | \
                                BIT(IIO_CHAN_INFO_SAMP_FREQ), \
    .scan_index = (idx),                               \
    .scan_type = {                                     \
        .sign = 's',                                   \
        .realbits = 16,                                \
        .storagebits = 16,                             \
        .endianness = IIO_LE,                          \
    },                                                 \
}

static const struct iio_chan_spec my_accel_channels[] = {
    MY_ACCEL_CHANNEL(X, 0),
    MY_ACCEL_CHANNEL(Y, 1),
    MY_ACCEL_CHANNEL(Z, 2),
    IIO_CHAN_SOFT_TIMESTAMP(3),
};
```

## Complete ADC Driver Example

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>

#define REG_CONVERSION  0x00
#define REG_CONFIG      0x01
#define REG_LO_THRESH   0x02
#define REG_HI_THRESH   0x03

#define NUM_CHANNELS    4

struct my_adc {
    struct regmap *regmap;
    struct regulator *vref;
    struct mutex lock;
    int vref_mv;
};

static const struct regmap_config my_adc_regmap_config = {
    .reg_bits = 8,
    .val_bits = 16,
    .max_register = REG_HI_THRESH,
};

static int my_adc_read_channel(struct my_adc *adc, int channel, int *val)
{
    unsigned int config, data;
    int ret;

    mutex_lock(&adc->lock);

    /* Select channel and start conversion */
    ret = regmap_read(adc->regmap, REG_CONFIG, &config);
    if (ret)
        goto out;

    config = (config & 0x0FFF) | (channel << 12) | BIT(15);
    ret = regmap_write(adc->regmap, REG_CONFIG, config);
    if (ret)
        goto out;

    /* Wait for conversion (simplified - should poll or use IRQ) */
    usleep_range(1000, 2000);

    /* Read result */
    ret = regmap_read(adc->regmap, REG_CONVERSION, &data);
    if (ret)
        goto out;

    *val = data >> 4;  /* 12-bit result */

out:
    mutex_unlock(&adc->lock);
    return ret;
}

static int my_adc_read_raw(struct iio_dev *indio_dev,
                           struct iio_chan_spec const *chan,
                           int *val, int *val2, long mask)
{
    struct my_adc *adc = iio_priv(indio_dev);
    int ret;

    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        ret = my_adc_read_channel(adc, chan->channel, val);
        if (ret)
            return ret;
        return IIO_VAL_INT;

    case IIO_CHAN_INFO_SCALE:
        /* Scale = Vref / 2^12 */
        *val = adc->vref_mv;
        *val2 = 12;
        return IIO_VAL_FRACTIONAL_LOG2;

    default:
        return -EINVAL;
    }
}

static const struct iio_chan_spec my_adc_channels[] = {
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 0,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 1,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 2,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 3,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
};

static const struct iio_info my_adc_info = {
    .read_raw = my_adc_read_raw,
};

static int my_adc_probe(struct i2c_client *client)
{
    struct my_adc *adc;
    struct iio_dev *indio_dev;
    int ret;

    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*adc));
    if (!indio_dev)
        return -ENOMEM;

    adc = iio_priv(indio_dev);
    mutex_init(&adc->lock);

    adc->regmap = devm_regmap_init_i2c(client, &my_adc_regmap_config);
    if (IS_ERR(adc->regmap))
        return PTR_ERR(adc->regmap);

    /* Get reference voltage */
    adc->vref = devm_regulator_get(&client->dev, "vref");
    if (IS_ERR(adc->vref))
        return PTR_ERR(adc->vref);

    ret = regulator_enable(adc->vref);
    if (ret)
        return ret;

    adc->vref_mv = regulator_get_voltage(adc->vref) / 1000;

    /* Setup IIO device */
    indio_dev->name = "my-adc";
    indio_dev->info = &my_adc_info;
    indio_dev->modes = INDIO_DIRECT_MODE;
    indio_dev->channels = my_adc_channels;
    indio_dev->num_channels = ARRAY_SIZE(my_adc_channels);

    ret = devm_iio_device_register(&client->dev, indio_dev);
    if (ret) {
        regulator_disable(adc->vref);
        return ret;
    }

    i2c_set_clientdata(client, indio_dev);

    dev_info(&client->dev, "ADC registered: %d channels, Vref=%d mV\n",
             NUM_CHANNELS, adc->vref_mv);

    return 0;
}

static void my_adc_remove(struct i2c_client *client)
{
    struct iio_dev *indio_dev = i2c_get_clientdata(client);
    struct my_adc *adc = iio_priv(indio_dev);

    regulator_disable(adc->vref);
}

static const struct of_device_id my_adc_of_match[] = {
    { .compatible = "vendor,my-adc" },
    { }
};
MODULE_DEVICE_TABLE(of, my_adc_of_match);

static struct i2c_driver my_adc_driver = {
    .driver = {
        .name = "my-adc",
        .of_match_table = my_adc_of_match,
    },
    .probe = my_adc_probe,
    .remove = my_adc_remove,
};
module_i2c_driver(my_adc_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Example IIO ADC Driver");
```

## Return Value Types

```c
switch (mask) {
case IIO_CHAN_INFO_RAW:
    *val = raw_value;
    return IIO_VAL_INT;              /* Integer value in val */

case IIO_CHAN_INFO_SCALE:
    /* val.val2 = val / 2^val2 */
    *val = 3300;                      /* 3.3V in mV */
    *val2 = 12;                       /* 12-bit ADC */
    return IIO_VAL_FRACTIONAL_LOG2;   /* val / 2^val2 */

case IIO_CHAN_INFO_SAMP_FREQ:
    *val = 1000;
    return IIO_VAL_INT;              /* 1000 Hz */

case IIO_CHAN_INFO_OFFSET:
    *val = 0;
    *val2 = 500000;                   /* 0.5 */
    return IIO_VAL_INT_PLUS_MICRO;    /* val + val2/1000000 */

case IIO_CHAN_INFO_CALIBSCALE:
    *val = 1;
    *val2 = 234;                      /* 1.000234 */
    return IIO_VAL_INT_PLUS_NANO;     /* val + val2/1000000000 */
}
```

## Available Attributes

```c
static IIO_CONST_ATTR(sampling_frequency_available, "100 200 400 800");

static IIO_CONST_ATTR_SAMP_FREQ_AVAIL("100 200 400 800");

static struct attribute *my_sensor_attrs[] = {
    &iio_const_attr_sampling_frequency_available.dev_attr.attr,
    NULL
};

static const struct attribute_group my_sensor_attrs_group = {
    .attrs = my_sensor_attrs,
};

static const struct iio_info my_sensor_info = {
    .read_raw = my_sensor_read_raw,
    .write_raw = my_sensor_write_raw,
    .attrs = &my_sensor_attrs_group,
};
```

## Write Raw Implementation

```c
static int my_sensor_write_raw(struct iio_dev *indio_dev,
                               struct iio_chan_spec const *chan,
                               int val, int val2, long mask)
{
    struct my_sensor *sensor = iio_priv(indio_dev);
    int ret;

    switch (mask) {
    case IIO_CHAN_INFO_SAMP_FREQ:
        if (val < 100 || val > 800)
            return -EINVAL;
        mutex_lock(&sensor->lock);
        ret = set_sampling_freq(sensor, val);
        mutex_unlock(&sensor->lock);
        return ret;

    case IIO_CHAN_INFO_CALIBBIAS:
        if (val < -32768 || val > 32767)
            return -EINVAL;
        return regmap_write(sensor->regmap,
                           REG_CALIBBIAS + chan->scan_index * 2,
                           val);

    default:
        return -EINVAL;
    }
}
```

## Device Tree Binding

```dts
&i2c1 {
    adc@48 {
        compatible = "vendor,my-adc";
        reg = <0x48>;
        #io-channel-cells = <1>;
        vref-supply = <&vdd_3v3>;
    };
};

/* Consumer */
battery_monitor {
    compatible = "simple-battery-monitor";
    io-channels = <&adc 0>, <&adc 1>;
    io-channel-names = "voltage", "current";
};
```

## Summary

- Use `devm_iio_device_alloc()` with private data size
- Define channels with `iio_chan_spec` array
- Implement `read_raw()` and optionally `write_raw()`
- Return appropriate `IIO_VAL_*` type
- Use `devm_iio_device_register()` for registration
- Handle reference voltage with regulator framework

## Further Reading

- [IIO Core](https://docs.kernel.org/driver-api/iio/core.html) - Core documentation
- [IIO Drivers](https://elixir.bootlin.com/linux/v6.6/source/drivers/iio/adc) - ADC examples
- [Channel Specification](https://docs.kernel.org/driver-api/iio/iio_chan_spec.html) - Channel details

## Next

Learn about [IIO Buffers]({% link part11/03-iio-buffers.md %}) for continuous data acquisition.
