// SPDX-License-Identifier: GPL-2.0
/*
 * I2C Sensor Driver Demo
 *
 * Demonstrates I2C client driver using regmap with IIO subsystem.
 * Simulates a temperature/humidity sensor like an HTU21D or SHT31.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/delay.h>

/* Register definitions (simulated sensor) */
#define REG_DEVICE_ID       0x00
#define REG_STATUS          0x01
#define REG_CONFIG          0x02
#define REG_TEMP_L          0x03
#define REG_TEMP_H          0x04
#define REG_HUMID_L         0x05
#define REG_HUMID_H         0x06
#define REG_CALIB_TEMP      0x10
#define REG_CALIB_HUMID     0x11

#define DEVICE_ID_VALUE     0x5A

/* Config register bits */
#define CFG_ENABLE          BIT(0)
#define CFG_TEMP_RES_HIGH   BIT(1)
#define CFG_HUMID_RES_HIGH  BIT(2)
#define CFG_HEATER_EN       BIT(3)

/* Status register bits */
#define STATUS_TEMP_READY   BIT(0)
#define STATUS_HUMID_READY  BIT(1)
#define STATUS_BUSY         BIT(7)

struct demo_sensor {
    struct device *dev;
    struct regmap *regmap;
    struct mutex lock;
    s8 temp_calibration;
    s8 humid_calibration;
};

/* Register defaults for cache */
static const struct reg_default demo_sensor_defaults[] = {
    { REG_CONFIG, 0x00 },
    { REG_CALIB_TEMP, 0x00 },
    { REG_CALIB_HUMID, 0x00 },
};

/* Volatile registers - always read from hardware */
static bool demo_sensor_volatile(struct device *dev, unsigned int reg)
{
    switch (reg) {
    case REG_STATUS:
    case REG_TEMP_L:
    case REG_TEMP_H:
    case REG_HUMID_L:
    case REG_HUMID_H:
        return true;
    default:
        return false;
    }
}

/* Readable register check */
static bool demo_sensor_readable(struct device *dev, unsigned int reg)
{
    return reg <= REG_CALIB_HUMID;
}

/* Writeable register check */
static bool demo_sensor_writeable(struct device *dev, unsigned int reg)
{
    switch (reg) {
    case REG_CONFIG:
    case REG_CALIB_TEMP:
    case REG_CALIB_HUMID:
        return true;
    default:
        return false;
    }
}

static const struct regmap_config demo_sensor_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = REG_CALIB_HUMID,
    .cache_type = REGCACHE_RBTREE,
    .reg_defaults = demo_sensor_defaults,
    .num_reg_defaults = ARRAY_SIZE(demo_sensor_defaults),
    .volatile_reg = demo_sensor_volatile,
    .readable_reg = demo_sensor_readable,
    .writeable_reg = demo_sensor_writeable,
};

static int demo_sensor_wait_ready(struct demo_sensor *sensor, unsigned int mask)
{
    unsigned int status;
    int ret;
    int timeout = 100;

    while (timeout--) {
        ret = regmap_read(sensor->regmap, REG_STATUS, &status);
        if (ret)
            return ret;

        if (status & mask)
            return 0;

        usleep_range(1000, 2000);
    }

    return -ETIMEDOUT;
}

static int demo_sensor_read_temp(struct demo_sensor *sensor, int *val)
{
    unsigned int temp_l, temp_h;
    s16 raw_temp;
    int ret;

    mutex_lock(&sensor->lock);

    /* Trigger conversion if needed */
    ret = regmap_update_bits(sensor->regmap, REG_CONFIG, CFG_ENABLE, CFG_ENABLE);
    if (ret)
        goto out;

    /* Wait for temperature ready */
    ret = demo_sensor_wait_ready(sensor, STATUS_TEMP_READY);
    if (ret)
        goto out;

    /* Read temperature registers */
    ret = regmap_read(sensor->regmap, REG_TEMP_L, &temp_l);
    if (ret)
        goto out;

    ret = regmap_read(sensor->regmap, REG_TEMP_H, &temp_h);
    if (ret)
        goto out;

    raw_temp = (s16)((temp_h << 8) | temp_l);

    /* Apply calibration offset (in 0.1°C units) */
    *val = raw_temp + (sensor->temp_calibration * 10);

out:
    mutex_unlock(&sensor->lock);
    return ret;
}

static int demo_sensor_read_humidity(struct demo_sensor *sensor, int *val)
{
    unsigned int humid_l, humid_h;
    u16 raw_humid;
    int ret;

    mutex_lock(&sensor->lock);

    /* Trigger conversion if needed */
    ret = regmap_update_bits(sensor->regmap, REG_CONFIG, CFG_ENABLE, CFG_ENABLE);
    if (ret)
        goto out;

    /* Wait for humidity ready */
    ret = demo_sensor_wait_ready(sensor, STATUS_HUMID_READY);
    if (ret)
        goto out;

    /* Read humidity registers */
    ret = regmap_read(sensor->regmap, REG_HUMID_L, &humid_l);
    if (ret)
        goto out;

    ret = regmap_read(sensor->regmap, REG_HUMID_H, &humid_h);
    if (ret)
        goto out;

    raw_humid = (humid_h << 8) | humid_l;

    /* Apply calibration offset (in 0.1% units) */
    *val = raw_humid + (sensor->humid_calibration * 10);

out:
    mutex_unlock(&sensor->lock);
    return ret;
}

static int demo_sensor_read_raw(struct iio_dev *indio_dev,
                                struct iio_chan_spec const *chan,
                                int *val, int *val2, long mask)
{
    struct demo_sensor *sensor = iio_priv(indio_dev);
    int ret;

    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        switch (chan->type) {
        case IIO_TEMP:
            ret = demo_sensor_read_temp(sensor, val);
            if (ret)
                return ret;
            return IIO_VAL_INT;

        case IIO_HUMIDITYRELATIVE:
            ret = demo_sensor_read_humidity(sensor, val);
            if (ret)
                return ret;
            return IIO_VAL_INT;

        default:
            return -EINVAL;
        }

    case IIO_CHAN_INFO_SCALE:
        /* Temperature: raw value in 0.01°C, scale to °C */
        /* Humidity: raw value in 0.01%, scale to % */
        *val = 0;
        *val2 = 10000;  /* 0.01 */
        return IIO_VAL_INT_PLUS_MICRO;

    case IIO_CHAN_INFO_OFFSET:
        if (chan->type == IIO_TEMP) {
            /* Temperature offset for Kelvin conversion */
            *val = 27315;  /* 273.15 * 100 */
            return IIO_VAL_INT;
        }
        return -EINVAL;

    default:
        return -EINVAL;
    }
}

static int demo_sensor_write_raw(struct iio_dev *indio_dev,
                                 struct iio_chan_spec const *chan,
                                 int val, int val2, long mask)
{
    struct demo_sensor *sensor = iio_priv(indio_dev);

    if (mask != IIO_CHAN_INFO_CALIBBIAS)
        return -EINVAL;

    /* Calibration value range: -128 to 127 */
    if (val < -128 || val > 127)
        return -EINVAL;

    mutex_lock(&sensor->lock);

    switch (chan->type) {
    case IIO_TEMP:
        sensor->temp_calibration = val;
        regmap_write(sensor->regmap, REG_CALIB_TEMP, val);
        break;

    case IIO_HUMIDITYRELATIVE:
        sensor->humid_calibration = val;
        regmap_write(sensor->regmap, REG_CALIB_HUMID, val);
        break;

    default:
        mutex_unlock(&sensor->lock);
        return -EINVAL;
    }

    mutex_unlock(&sensor->lock);
    return 0;
}

static const struct iio_chan_spec demo_sensor_channels[] = {
    {
        .type = IIO_TEMP,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
                              BIT(IIO_CHAN_INFO_CALIBBIAS),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) |
                                    BIT(IIO_CHAN_INFO_OFFSET),
    },
    {
        .type = IIO_HUMIDITYRELATIVE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
                              BIT(IIO_CHAN_INFO_CALIBBIAS),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
};

static const struct iio_info demo_sensor_info = {
    .read_raw = demo_sensor_read_raw,
    .write_raw = demo_sensor_write_raw,
};

static int demo_sensor_probe(struct i2c_client *client)
{
    struct demo_sensor *sensor;
    struct iio_dev *indio_dev;
    unsigned int device_id;
    int ret;

    /* Allocate IIO device with private data */
    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*sensor));
    if (!indio_dev)
        return -ENOMEM;

    sensor = iio_priv(indio_dev);
    sensor->dev = &client->dev;
    mutex_init(&sensor->lock);

    /* Initialize regmap */
    sensor->regmap = devm_regmap_init_i2c(client, &demo_sensor_regmap_config);
    if (IS_ERR(sensor->regmap))
        return dev_err_probe(&client->dev, PTR_ERR(sensor->regmap),
                             "Failed to initialize regmap\n");

    /* Verify device ID */
    ret = regmap_read(sensor->regmap, REG_DEVICE_ID, &device_id);
    if (ret)
        return dev_err_probe(&client->dev, ret,
                             "Failed to read device ID\n");

    if (device_id != DEVICE_ID_VALUE) {
        dev_err(&client->dev, "Unknown device ID: 0x%02x (expected 0x%02x)\n",
                device_id, DEVICE_ID_VALUE);
        return -ENODEV;
    }

    /* Configure device: enable with high resolution */
    ret = regmap_write(sensor->regmap, REG_CONFIG,
                       CFG_ENABLE | CFG_TEMP_RES_HIGH | CFG_HUMID_RES_HIGH);
    if (ret)
        return ret;

    /* Read calibration values from device */
    ret = regmap_read(sensor->regmap, REG_CALIB_TEMP,
                      (unsigned int *)&sensor->temp_calibration);
    if (ret)
        sensor->temp_calibration = 0;

    ret = regmap_read(sensor->regmap, REG_CALIB_HUMID,
                      (unsigned int *)&sensor->humid_calibration);
    if (ret)
        sensor->humid_calibration = 0;

    /* Setup IIO device */
    indio_dev->name = "demo-sensor";
    indio_dev->info = &demo_sensor_info;
    indio_dev->channels = demo_sensor_channels;
    indio_dev->num_channels = ARRAY_SIZE(demo_sensor_channels);
    indio_dev->modes = INDIO_DIRECT_MODE;

    i2c_set_clientdata(client, indio_dev);

    ret = devm_iio_device_register(&client->dev, indio_dev);
    if (ret)
        return ret;

    dev_info(&client->dev, "Demo I2C sensor initialized (ID: 0x%02x)\n",
             device_id);

    return 0;
}

static void demo_sensor_remove(struct i2c_client *client)
{
    struct iio_dev *indio_dev = i2c_get_clientdata(client);
    struct demo_sensor *sensor = iio_priv(indio_dev);

    /* Put device in low-power mode */
    regmap_write(sensor->regmap, REG_CONFIG, 0);

    dev_info(&client->dev, "Demo I2C sensor removed\n");
}

static int demo_sensor_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct iio_dev *indio_dev = i2c_get_clientdata(client);
    struct demo_sensor *sensor = iio_priv(indio_dev);

    /* Disable sensor */
    regmap_write(sensor->regmap, REG_CONFIG, 0);
    regcache_mark_dirty(sensor->regmap);

    return 0;
}

static int demo_sensor_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct iio_dev *indio_dev = i2c_get_clientdata(client);
    struct demo_sensor *sensor = iio_priv(indio_dev);

    /* Restore configuration from cache */
    regcache_sync(sensor->regmap);

    /* Re-enable sensor */
    regmap_write(sensor->regmap, REG_CONFIG,
                 CFG_ENABLE | CFG_TEMP_RES_HIGH | CFG_HUMID_RES_HIGH);

    return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(demo_sensor_pm_ops,
                                demo_sensor_suspend,
                                demo_sensor_resume);

static const struct i2c_device_id demo_sensor_id[] = {
    { "demo-sensor", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, demo_sensor_id);

static const struct of_device_id demo_sensor_of_match[] = {
    { .compatible = "demo,i2c-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_sensor_of_match);

static struct i2c_driver demo_sensor_driver = {
    .driver = {
        .name = "demo-sensor",
        .of_match_table = demo_sensor_of_match,
        .pm = pm_sleep_ptr(&demo_sensor_pm_ops),
    },
    .probe = demo_sensor_probe,
    .remove = demo_sensor_remove,
    .id_table = demo_sensor_id,
};
module_i2c_driver(demo_sensor_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Demo I2C Sensor Driver with Regmap and IIO");
