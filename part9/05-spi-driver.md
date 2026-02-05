---
layout: default
title: "9.5 SPI Driver"
parent: "Part 9: I2C, SPI, and GPIO Drivers"
nav_order: 5
---

# SPI Driver

This chapter covers writing drivers for SPI devices. SPI drivers follow a similar pattern to I2C drivers.

## SPI Driver Structure

```c
#include <linux/spi/spi.h>

static int my_probe(struct spi_device *spi);
static void my_remove(struct spi_device *spi);

static const struct spi_device_id my_id_table[] = {
    { "my-spi-device", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, my_id_table);

static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,my-spi-device" },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);

static struct spi_driver my_driver = {
    .driver = {
        .name = "my-spi-device",
        .of_match_table = my_of_match,
    },
    .probe = my_probe,
    .remove = my_remove,
    .id_table = my_id_table,
};

module_spi_driver(my_driver);
```

## Complete Example: SPI Sensor

```c
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

/* Register definitions */
#define REG_WHO_AM_I    0x0F
#define REG_CTRL1       0x20
#define REG_CTRL2       0x21
#define REG_STATUS      0x27
#define REG_DATA_X_L    0x28
#define REG_DATA_X_H    0x29
#define REG_DATA_Y_L    0x2A
#define REG_DATA_Y_H    0x2B
#define REG_DATA_Z_L    0x2C
#define REG_DATA_Z_H    0x2D

#define WHO_AM_I_VALUE  0x33

/* SPI protocol: bit 7 = read, bit 6 = auto-increment */
#define SPI_READ        0x80
#define SPI_AUTO_INC    0x40

struct my_sensor {
    struct spi_device *spi;
    struct regmap *regmap;
    struct mutex lock;
};

/* Regmap configuration for SPI */
static const struct regmap_config my_sensor_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .read_flag_mask = SPI_READ,
    .max_register = 0x3F,
};

static int my_sensor_read_axis(struct my_sensor *sensor, int axis, int *val)
{
    u8 data[2];
    int ret;
    u8 reg = REG_DATA_X_L + (axis * 2);

    /* Read two bytes with auto-increment */
    ret = regmap_bulk_read(sensor->regmap, reg | SPI_AUTO_INC, data, 2);
    if (ret)
        return ret;

    /* Convert to signed 16-bit */
    *val = (s16)((data[1] << 8) | data[0]);
    return 0;
}

static int my_sensor_read_raw(struct iio_dev *indio_dev,
                              struct iio_chan_spec const *chan,
                              int *val, int *val2, long mask)
{
    struct my_sensor *sensor = iio_priv(indio_dev);
    int ret;

    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        mutex_lock(&sensor->lock);
        ret = my_sensor_read_axis(sensor, chan->scan_index, val);
        mutex_unlock(&sensor->lock);
        if (ret)
            return ret;
        return IIO_VAL_INT;

    case IIO_CHAN_INFO_SCALE:
        /* 2g range, 16-bit signed = 0.061 mg/LSB */
        *val = 0;
        *val2 = 598;  /* 0.000598 m/s^2 per LSB */
        return IIO_VAL_INT_PLUS_MICRO;

    default:
        return -EINVAL;
    }
}

#define MY_CHANNEL(axis, idx) {                    \
    .type = IIO_ACCEL,                             \
    .modified = 1,                                 \
    .channel2 = IIO_MOD_##axis,                    \
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW), \
    .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE), \
    .scan_index = idx,                             \
}

static const struct iio_chan_spec my_sensor_channels[] = {
    MY_CHANNEL(X, 0),
    MY_CHANNEL(Y, 1),
    MY_CHANNEL(Z, 2),
};

static const struct iio_info my_sensor_info = {
    .read_raw = my_sensor_read_raw,
};

static int my_sensor_probe(struct spi_device *spi)
{
    struct my_sensor *sensor;
    struct iio_dev *indio_dev;
    unsigned int who_am_i;
    int ret;

    /* Verify SPI mode */
    spi->mode = SPI_MODE_3;
    spi->bits_per_word = 8;
    ret = spi_setup(spi);
    if (ret)
        return ret;

    /* Allocate IIO device */
    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*sensor));
    if (!indio_dev)
        return -ENOMEM;

    sensor = iio_priv(indio_dev);
    sensor->spi = spi;
    mutex_init(&sensor->lock);

    /* Initialize regmap */
    sensor->regmap = devm_regmap_init_spi(spi, &my_sensor_regmap_config);
    if (IS_ERR(sensor->regmap))
        return PTR_ERR(sensor->regmap);

    /* Verify device ID */
    ret = regmap_read(sensor->regmap, REG_WHO_AM_I, &who_am_i);
    if (ret)
        return dev_err_probe(&spi->dev, ret, "Failed to read WHO_AM_I\n");

    if (who_am_i != WHO_AM_I_VALUE) {
        dev_err(&spi->dev, "Wrong device ID: 0x%02x\n", who_am_i);
        return -ENODEV;
    }

    /* Configure device */
    ret = regmap_write(sensor->regmap, REG_CTRL1, 0x47);  /* 50Hz, enable XYZ */
    if (ret)
        return ret;

    /* Setup IIO device */
    indio_dev->name = spi->modalias;
    indio_dev->info = &my_sensor_info;
    indio_dev->channels = my_sensor_channels;
    indio_dev->num_channels = ARRAY_SIZE(my_sensor_channels);
    indio_dev->modes = INDIO_DIRECT_MODE;

    spi_set_drvdata(spi, indio_dev);

    ret = devm_iio_device_register(&spi->dev, indio_dev);
    if (ret)
        return ret;

    dev_info(&spi->dev, "Sensor initialized (WHO_AM_I=0x%02x)\n", who_am_i);
    return 0;
}

static void my_sensor_remove(struct spi_device *spi)
{
    struct iio_dev *indio_dev = spi_get_drvdata(spi);
    struct my_sensor *sensor = iio_priv(indio_dev);

    /* Put device in power-down mode */
    regmap_write(sensor->regmap, REG_CTRL1, 0x00);
}

static const struct spi_device_id my_sensor_id[] = {
    { "my-accel", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, my_sensor_id);

static const struct of_device_id my_sensor_of_match[] = {
    { .compatible = "vendor,my-accel" },
    { }
};
MODULE_DEVICE_TABLE(of, my_sensor_of_match);

static struct spi_driver my_sensor_driver = {
    .driver = {
        .name = "my-accel",
        .of_match_table = my_sensor_of_match,
    },
    .probe = my_sensor_probe,
    .remove = my_sensor_remove,
    .id_table = my_sensor_id,
};
module_spi_driver(my_sensor_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("SPI Accelerometer Driver");
```

## SPI Communication Without Regmap

```c
/* Read single register */
static int read_reg(struct spi_device *spi, u8 reg, u8 *val)
{
    u8 tx = reg | 0x80;  /* Read bit */
    return spi_write_then_read(spi, &tx, 1, val, 1);
}

/* Write single register */
static int write_reg(struct spi_device *spi, u8 reg, u8 val)
{
    u8 tx[2] = { reg & 0x7F, val };  /* Clear read bit */
    return spi_write(spi, tx, 2);
}

/* Read multiple registers */
static int read_regs(struct spi_device *spi, u8 reg, u8 *buf, int len)
{
    u8 cmd = reg | 0x80 | 0x40;  /* Read + auto-increment */
    struct spi_transfer t[] = {
        { .tx_buf = &cmd, .len = 1 },
        { .rx_buf = buf, .len = len },
    };

    return spi_sync_transfer(spi, t, ARRAY_SIZE(t));
}

/* Full-duplex transfer */
static int transfer(struct spi_device *spi, u8 *tx, u8 *rx, int len)
{
    struct spi_transfer t = {
        .tx_buf = tx,
        .rx_buf = rx,
        .len = len,
    };

    return spi_sync_transfer(spi, &t, 1);
}
```

## Device Tree Binding

```dts
&spi1 {
    status = "okay";

    accel: accelerometer@0 {
        compatible = "vendor,my-accel";
        reg = <0>;                      /* CS0 */
        spi-max-frequency = <10000000>; /* 10 MHz */
        spi-cpol;                       /* Mode 2 or 3 */
        spi-cpha;                       /* Mode 1 or 3 */

        interrupt-parent = <&gpio>;
        interrupts = <5 IRQ_TYPE_EDGE_RISING>;
    };
};
```

## SPI Setup in Probe

```c
static int my_probe(struct spi_device *spi)
{
    int ret;

    /* Configure SPI parameters */
    spi->mode = SPI_MODE_3;       /* CPOL=1, CPHA=1 */
    spi->bits_per_word = 8;
    spi->max_speed_hz = 10000000; /* 10 MHz */

    ret = spi_setup(spi);
    if (ret) {
        dev_err(&spi->dev, "SPI setup failed: %d\n", ret);
        return ret;
    }

    /* Get actual speed */
    dev_info(&spi->dev, "SPI speed: %d Hz\n", spi->max_speed_hz);

    return 0;
}
```

## Handling CS Manually

For complex protocols requiring manual CS control:

```c
static int complex_transfer(struct spi_device *spi)
{
    struct spi_transfer t1 = {
        .tx_buf = cmd,
        .len = 1,
        .cs_change = 1,  /* Keep CS asserted */
    };
    struct spi_transfer t2 = {
        .rx_buf = data,
        .len = 4,
        .cs_change = 0,  /* Deassert CS after */
    };
    struct spi_message m;

    spi_message_init(&m);
    spi_message_add_tail(&t1, &m);
    spi_message_add_tail(&t2, &m);

    return spi_sync(spi, &m);
}
```

## Power Management

```c
static int my_sensor_suspend(struct device *dev)
{
    struct spi_device *spi = to_spi_device(dev);
    struct my_sensor *sensor = spi_get_drvdata(spi);

    /* Put device in low-power mode */
    regmap_write(sensor->regmap, REG_CTRL1, 0x00);
    return 0;
}

static int my_sensor_resume(struct device *dev)
{
    struct spi_device *spi = to_spi_device(dev);
    struct my_sensor *sensor = spi_get_drvdata(spi);

    /* Restore normal operation */
    regmap_write(sensor->regmap, REG_CTRL1, 0x47);
    return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(my_sensor_pm_ops,
                                my_sensor_suspend,
                                my_sensor_resume);

static struct spi_driver my_sensor_driver = {
    .driver = {
        .name = "my-accel",
        .pm = pm_sleep_ptr(&my_sensor_pm_ops),
        .of_match_table = my_sensor_of_match,
    },
    /* ... */
};
```

## Summary

- Use `module_spi_driver()` for simple registration
- Configure mode, bits_per_word, max_speed_hz
- Call `spi_setup()` after changing parameters
- Use regmap for register-based devices
- Use `spi_sync_transfer()` for raw transfers
- Handle CS with `cs_change` flag for complex protocols

## Next

Learn about the [GPIO consumer API]({% link part9/06-gpio-consumer.md %}).
