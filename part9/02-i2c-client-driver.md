---
layout: default
title: "9.2 I2C Client Driver"
parent: "Part 9: I2C, SPI, and GPIO Drivers"
nav_order: 2
---

# I2C Client Driver

This chapter covers writing drivers for I2C devices (clients). The driver registers with the I2C subsystem and handles device probe/remove.

## I2C Driver Structure

```c
#include <linux/i2c.h>

static int my_probe(struct i2c_client *client);
static void my_remove(struct i2c_client *client);

static const struct i2c_device_id my_id_table[] = {
    { "my-sensor", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, my_id_table);

static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,my-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);

static struct i2c_driver my_driver = {
    .driver = {
        .name = "my-sensor",
        .of_match_table = my_of_match,
    },
    .probe = my_probe,
    .remove = my_remove,
    .id_table = my_id_table,
};

module_i2c_driver(my_driver);
```

## Complete Example: Temperature Sensor

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>

#define TEMP_REG    0x00
#define CONFIG_REG  0x01
#define TLOW_REG    0x02
#define THIGH_REG   0x03

struct my_sensor {
    struct i2c_client *client;
    struct mutex lock;
};

/* Read temperature register */
static int my_sensor_read_temp(struct my_sensor *sensor)
{
    struct i2c_client *client = sensor->client;
    int ret;
    u8 buf[2];

    mutex_lock(&sensor->lock);

    ret = i2c_smbus_read_word_swapped(client, TEMP_REG);

    mutex_unlock(&sensor->lock);

    if (ret < 0)
        return ret;

    /* Convert to millidegrees Celsius */
    /* Assuming 12-bit resolution, 0.0625°C per LSB */
    return (ret >> 4) * 625 / 10;
}

/* hwmon read callback */
static int my_sensor_hwmon_read(struct device *dev,
                                enum hwmon_sensor_types type,
                                u32 attr, int channel, long *val)
{
    struct my_sensor *sensor = dev_get_drvdata(dev);
    int ret;

    if (type != hwmon_temp || attr != hwmon_temp_input)
        return -EOPNOTSUPP;

    ret = my_sensor_read_temp(sensor);
    if (ret < 0)
        return ret;

    *val = ret;
    return 0;
}

static umode_t my_sensor_is_visible(const void *data,
                                    enum hwmon_sensor_types type,
                                    u32 attr, int channel)
{
    if (type == hwmon_temp && attr == hwmon_temp_input)
        return 0444;
    return 0;
}

static const struct hwmon_channel_info *my_sensor_info[] = {
    HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT),
    NULL
};

static const struct hwmon_ops my_sensor_hwmon_ops = {
    .is_visible = my_sensor_is_visible,
    .read = my_sensor_hwmon_read,
};

static const struct hwmon_chip_info my_sensor_chip_info = {
    .ops = &my_sensor_hwmon_ops,
    .info = my_sensor_info,
};

static int my_sensor_probe(struct i2c_client *client)
{
    struct my_sensor *sensor;
    struct device *hwmon_dev;
    int ret;

    /* Check adapter functionality */
    if (!i2c_check_functionality(client->adapter,
                                  I2C_FUNC_SMBUS_WORD_DATA)) {
        dev_err(&client->dev, "Adapter doesn't support SMBus word\n");
        return -ENODEV;
    }

    sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return -ENOMEM;

    sensor->client = client;
    mutex_init(&sensor->lock);

    /* Verify device responds */
    ret = i2c_smbus_read_byte_data(client, CONFIG_REG);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read config: %d\n", ret);
        return ret;
    }

    /* Configure device */
    ret = i2c_smbus_write_byte_data(client, CONFIG_REG, 0x60);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to write config: %d\n", ret);
        return ret;
    }

    i2c_set_clientdata(client, sensor);

    /* Register with hwmon */
    hwmon_dev = devm_hwmon_device_register_with_info(&client->dev,
                    client->name, sensor,
                    &my_sensor_chip_info, NULL);
    if (IS_ERR(hwmon_dev))
        return PTR_ERR(hwmon_dev);

    dev_info(&client->dev, "Temperature sensor initialized\n");
    return 0;
}

static void my_sensor_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "Temperature sensor removed\n");
}

static const struct i2c_device_id my_sensor_id[] = {
    { "my-temp-sensor", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, my_sensor_id);

static const struct of_device_id my_sensor_of_match[] = {
    { .compatible = "vendor,my-temp-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, my_sensor_of_match);

static struct i2c_driver my_sensor_driver = {
    .driver = {
        .name = "my-temp-sensor",
        .of_match_table = my_sensor_of_match,
    },
    .probe = my_sensor_probe,
    .remove = my_sensor_remove,
    .id_table = my_sensor_id,
};

module_i2c_driver(my_sensor_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("I2C Temperature Sensor Driver");
```

## I2C Communication Patterns

### Read Single Register

```c
/* Using SMBus API */
int value = i2c_smbus_read_byte_data(client, reg);
if (value < 0)
    return value;  /* Error */

/* Using raw I2C */
u8 reg_addr = REG;
u8 data;
struct i2c_msg msgs[] = {
    { .addr = client->addr, .flags = 0, .len = 1, .buf = &reg_addr },
    { .addr = client->addr, .flags = I2C_M_RD, .len = 1, .buf = &data },
};

ret = i2c_transfer(client->adapter, msgs, 2);
if (ret != 2)
    return -EIO;
```

### Write Single Register

```c
/* Using SMBus API */
ret = i2c_smbus_write_byte_data(client, reg, value);
if (ret < 0)
    return ret;

/* Using raw I2C */
u8 buf[2] = { reg, value };
ret = i2c_master_send(client, buf, 2);
if (ret != 2)
    return -EIO;
```

### Read Multiple Bytes

```c
/* Using SMBus block read */
u8 data[32];
ret = i2c_smbus_read_i2c_block_data(client, reg, len, data);
if (ret < 0)
    return ret;

/* Using raw I2C */
u8 reg_addr = REG;
struct i2c_msg msgs[] = {
    { .addr = client->addr, .flags = 0, .len = 1, .buf = &reg_addr },
    { .addr = client->addr, .flags = I2C_M_RD, .len = len, .buf = data },
};
ret = i2c_transfer(client->adapter, msgs, 2);
```

### 16-bit Register Values

```c
/* Big-endian (MSB first) - common for sensors */
int val = i2c_smbus_read_word_swapped(client, reg);

/* Little-endian (LSB first) */
int val = i2c_smbus_read_word_data(client, reg);

/* Write 16-bit */
ret = i2c_smbus_write_word_swapped(client, reg, value);
```

## Checking Adapter Functionality

```c
/* Check before using specific features */
if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
    dev_err(&client->dev, "SMBus byte data not supported\n");
    return -ENODEV;
}

/* Common functionality flags */
I2C_FUNC_I2C                    /* Plain I2C */
I2C_FUNC_SMBUS_BYTE             /* SMBus send/receive byte */
I2C_FUNC_SMBUS_BYTE_DATA        /* SMBus read/write byte data */
I2C_FUNC_SMBUS_WORD_DATA        /* SMBus read/write word data */
I2C_FUNC_SMBUS_BLOCK_DATA       /* SMBus block read/write */
I2C_FUNC_SMBUS_I2C_BLOCK        /* I2C block transfers */
```

## Device Tree Binding

```dts
&i2c1 {
    status = "okay";
    clock-frequency = <400000>;

    temp_sensor: sensor@48 {
        compatible = "vendor,my-temp-sensor";
        reg = <0x48>;
        /* Optional: interrupt */
        interrupt-parent = <&gpio>;
        interrupts = <5 IRQ_TYPE_EDGE_FALLING>;
    };
};
```

## Power Management

```c
static int my_sensor_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct my_sensor *sensor = i2c_get_clientdata(client);

    /* Put device in low-power mode */
    i2c_smbus_write_byte_data(client, CONFIG_REG, 0x01);

    return 0;
}

static int my_sensor_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct my_sensor *sensor = i2c_get_clientdata(client);

    /* Restore normal operation */
    i2c_smbus_write_byte_data(client, CONFIG_REG, 0x60);

    return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(my_sensor_pm_ops,
                                my_sensor_suspend,
                                my_sensor_resume);

static struct i2c_driver my_sensor_driver = {
    .driver = {
        .name = "my-temp-sensor",
        .pm = pm_sleep_ptr(&my_sensor_pm_ops),
        .of_match_table = my_sensor_of_match,
    },
    /* ... */
};
```

## Error Handling

```c
static int my_read_reg(struct i2c_client *client, u8 reg, u8 *val)
{
    int ret;
    int retries = 3;

    while (retries--) {
        ret = i2c_smbus_read_byte_data(client, reg);
        if (ret >= 0) {
            *val = ret;
            return 0;
        }

        if (ret != -EAGAIN)
            break;

        usleep_range(1000, 2000);
    }

    dev_err(&client->dev, "Failed to read reg 0x%02x: %d\n", reg, ret);
    return ret;
}
```

## Summary

- Use `module_i2c_driver()` for simple registration
- Provide both `of_device_id` and `i2c_device_id` tables
- Check adapter functionality before using features
- Use SMBus functions for standard operations
- Use raw `i2c_transfer()` for custom protocols
- Handle endianness for multi-byte values
- Implement power management for battery devices

## Next

Learn about [regmap for I2C]({% link part9/03-i2c-regmap.md %}) to simplify register access.
