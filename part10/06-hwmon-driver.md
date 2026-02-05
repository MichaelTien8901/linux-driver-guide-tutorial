---
layout: default
title: "10.6 HWMON Driver"
parent: "Part 10: PWM, Watchdog, HWMON, LED Drivers"
nav_order: 6
---

# HWMON Driver

This chapter covers implementing hardware monitoring drivers using the `hwmon_chip_info` structure.

## Basic HWMON Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/io.h>

#define REG_TEMP        0x00
#define REG_VOLTAGE     0x04
#define REG_FAN_SPEED   0x08

struct my_hwmon {
    void __iomem *regs;
    struct device *hwmon_dev;
};

static int my_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
                         u32 attr, int channel, long *val)
{
    struct my_hwmon *hwmon = dev_get_drvdata(dev);

    switch (type) {
    case hwmon_temp:
        if (attr == hwmon_temp_input) {
            /* Read temperature, convert to millidegrees */
            u32 raw = readl(hwmon->regs + REG_TEMP);
            *val = (raw * 1000) / 256;  /* Example conversion */
            return 0;
        }
        break;

    case hwmon_in:
        if (attr == hwmon_in_input && channel == 0) {
            /* Read voltage in millivolts */
            u32 raw = readl(hwmon->regs + REG_VOLTAGE);
            *val = (raw * 3300) / 4096;  /* 3.3V reference, 12-bit ADC */
            return 0;
        }
        break;

    case hwmon_fan:
        if (attr == hwmon_fan_input && channel == 0) {
            /* Read fan speed in RPM */
            u32 raw = readl(hwmon->regs + REG_FAN_SPEED);
            *val = raw ? (60 * 1000000) / raw : 0;  /* Convert period to RPM */
            return 0;
        }
        break;

    default:
        break;
    }

    return -EOPNOTSUPP;
}

static umode_t my_hwmon_is_visible(const void *data,
                                   enum hwmon_sensor_types type,
                                   u32 attr, int channel)
{
    switch (type) {
    case hwmon_temp:
        if (attr == hwmon_temp_input)
            return 0444;
        break;

    case hwmon_in:
        if (channel == 0 && attr == hwmon_in_input)
            return 0444;
        break;

    case hwmon_fan:
        if (channel == 0 && attr == hwmon_fan_input)
            return 0444;
        break;

    default:
        break;
    }

    return 0;
}

static const struct hwmon_channel_info * const my_hwmon_info[] = {
    HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT),
    HWMON_CHANNEL_INFO(in, HWMON_I_INPUT),
    HWMON_CHANNEL_INFO(fan, HWMON_F_INPUT),
    NULL
};

static const struct hwmon_ops my_hwmon_ops = {
    .is_visible = my_hwmon_is_visible,
    .read = my_hwmon_read,
};

static const struct hwmon_chip_info my_hwmon_chip_info = {
    .ops = &my_hwmon_ops,
    .info = my_hwmon_info,
};

static int my_hwmon_probe(struct platform_device *pdev)
{
    struct my_hwmon *hwmon;
    struct device *hwmon_dev;

    hwmon = devm_kzalloc(&pdev->dev, sizeof(*hwmon), GFP_KERNEL);
    if (!hwmon)
        return -ENOMEM;

    hwmon->regs = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(hwmon->regs))
        return PTR_ERR(hwmon->regs);

    hwmon_dev = devm_hwmon_device_register_with_info(&pdev->dev,
                                                     "my_hwmon",
                                                     hwmon,
                                                     &my_hwmon_chip_info,
                                                     NULL);
    if (IS_ERR(hwmon_dev))
        return PTR_ERR(hwmon_dev);

    hwmon->hwmon_dev = hwmon_dev;
    platform_set_drvdata(pdev, hwmon);

    return 0;
}

static const struct of_device_id my_hwmon_of_match[] = {
    { .compatible = "vendor,my-hwmon" },
    { }
};
MODULE_DEVICE_TABLE(of, my_hwmon_of_match);

static struct platform_driver my_hwmon_driver = {
    .probe = my_hwmon_probe,
    .driver = {
        .name = "my-hwmon",
        .of_match_table = my_hwmon_of_match,
    },
};
module_platform_driver(my_hwmon_driver);

MODULE_LICENSE("GPL");
```

## Multi-Channel Temperature Sensor

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/regmap.h>

#define NUM_TEMP_CHANNELS   4

#define REG_TEMP_BASE       0x00
#define REG_TEMP_MAX_BASE   0x10
#define REG_TEMP_CRIT_BASE  0x20
#define REG_CONFIG          0x30

struct multi_temp_sensor {
    struct regmap *regmap;
    const char *channel_labels[NUM_TEMP_CHANNELS];
};

static int multi_temp_read(struct device *dev, enum hwmon_sensor_types type,
                           u32 attr, int channel, long *val)
{
    struct multi_temp_sensor *sensor = dev_get_drvdata(dev);
    unsigned int regval;
    int ret;
    u8 reg;

    if (type != hwmon_temp || channel >= NUM_TEMP_CHANNELS)
        return -EOPNOTSUPP;

    switch (attr) {
    case hwmon_temp_input:
        reg = REG_TEMP_BASE + channel;
        break;
    case hwmon_temp_max:
        reg = REG_TEMP_MAX_BASE + channel;
        break;
    case hwmon_temp_crit:
        reg = REG_TEMP_CRIT_BASE + channel;
        break;
    default:
        return -EOPNOTSUPP;
    }

    ret = regmap_read(sensor->regmap, reg, &regval);
    if (ret)
        return ret;

    /* Convert to millidegrees (assuming 8-bit signed, 1°C resolution) */
    *val = (s8)regval * 1000;

    return 0;
}

static int multi_temp_write(struct device *dev, enum hwmon_sensor_types type,
                            u32 attr, int channel, long val)
{
    struct multi_temp_sensor *sensor = dev_get_drvdata(dev);
    u8 reg;

    if (type != hwmon_temp || channel >= NUM_TEMP_CHANNELS)
        return -EOPNOTSUPP;

    /* Clamp and convert from millidegrees to register value */
    val = clamp_val(val, -128000, 127000);
    val = DIV_ROUND_CLOSEST(val, 1000);

    switch (attr) {
    case hwmon_temp_max:
        reg = REG_TEMP_MAX_BASE + channel;
        break;
    case hwmon_temp_crit:
        reg = REG_TEMP_CRIT_BASE + channel;
        break;
    default:
        return -EOPNOTSUPP;
    }

    return regmap_write(sensor->regmap, reg, val);
}

static int multi_temp_read_string(struct device *dev,
                                  enum hwmon_sensor_types type,
                                  u32 attr, int channel, const char **str)
{
    struct multi_temp_sensor *sensor = dev_get_drvdata(dev);

    if (type != hwmon_temp || attr != hwmon_temp_label)
        return -EOPNOTSUPP;

    if (channel >= NUM_TEMP_CHANNELS)
        return -EOPNOTSUPP;

    *str = sensor->channel_labels[channel];
    return 0;
}

static umode_t multi_temp_is_visible(const void *data,
                                     enum hwmon_sensor_types type,
                                     u32 attr, int channel)
{
    if (type != hwmon_temp || channel >= NUM_TEMP_CHANNELS)
        return 0;

    switch (attr) {
    case hwmon_temp_input:
    case hwmon_temp_label:
        return 0444;
    case hwmon_temp_max:
    case hwmon_temp_crit:
        return 0644;  /* Read/write */
    default:
        return 0;
    }
}

/* Define all 4 temperature channels with same attributes */
static const u32 multi_temp_config[] = {
    HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT | HWMON_T_LABEL,
    HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT | HWMON_T_LABEL,
    HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT | HWMON_T_LABEL,
    HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT | HWMON_T_LABEL,
    0
};

static const struct hwmon_channel_info multi_temp_channel = {
    .type = hwmon_temp,
    .config = multi_temp_config,
};

static const struct hwmon_channel_info * const multi_temp_info[] = {
    &multi_temp_channel,
    NULL
};

static const struct hwmon_ops multi_temp_ops = {
    .is_visible = multi_temp_is_visible,
    .read = multi_temp_read,
    .read_string = multi_temp_read_string,
    .write = multi_temp_write,
};

static const struct hwmon_chip_info multi_temp_chip_info = {
    .ops = &multi_temp_ops,
    .info = multi_temp_info,
};

static const struct regmap_config multi_temp_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x3F,
};

static int multi_temp_probe(struct i2c_client *client)
{
    struct multi_temp_sensor *sensor;
    struct device *hwmon_dev;

    sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return -ENOMEM;

    sensor->regmap = devm_regmap_init_i2c(client, &multi_temp_regmap_config);
    if (IS_ERR(sensor->regmap))
        return PTR_ERR(sensor->regmap);

    /* Set channel labels */
    sensor->channel_labels[0] = "CPU";
    sensor->channel_labels[1] = "GPU";
    sensor->channel_labels[2] = "Memory";
    sensor->channel_labels[3] = "Ambient";

    hwmon_dev = devm_hwmon_device_register_with_info(&client->dev,
                                                     "multi_temp",
                                                     sensor,
                                                     &multi_temp_chip_info,
                                                     NULL);
    return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct i2c_device_id multi_temp_id[] = {
    { "multi-temp", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, multi_temp_id);

static const struct of_device_id multi_temp_of_match[] = {
    { .compatible = "vendor,multi-temp" },
    { }
};
MODULE_DEVICE_TABLE(of, multi_temp_of_match);

static struct i2c_driver multi_temp_driver = {
    .driver = {
        .name = "multi-temp",
        .of_match_table = multi_temp_of_match,
    },
    .probe = multi_temp_probe,
    .id_table = multi_temp_id,
};
module_i2c_driver(multi_temp_driver);

MODULE_LICENSE("GPL");
```

## Power Monitor with Current/Voltage/Power

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/regmap.h>

#define REG_VOLTAGE     0x00    /* Bus voltage */
#define REG_CURRENT     0x02    /* Current */
#define REG_POWER       0x04    /* Power */
#define REG_CONFIG      0x06

struct power_monitor {
    struct regmap *regmap;
    u32 shunt_resistor;  /* micro-ohms */
    u32 voltage_lsb;     /* microvolts per LSB */
    u32 current_lsb;     /* microamps per LSB */
};

static int power_monitor_read(struct device *dev, enum hwmon_sensor_types type,
                              u32 attr, int channel, long *val)
{
    struct power_monitor *pm = dev_get_drvdata(dev);
    unsigned int regval;
    int ret;

    switch (type) {
    case hwmon_in:
        if (channel != 0 || attr != hwmon_in_input)
            return -EOPNOTSUPP;

        ret = regmap_read(pm->regmap, REG_VOLTAGE, &regval);
        if (ret)
            return ret;

        /* Convert to millivolts */
        *val = (regval * pm->voltage_lsb) / 1000;
        return 0;

    case hwmon_curr:
        if (channel != 0 || attr != hwmon_curr_input)
            return -EOPNOTSUPP;

        ret = regmap_read(pm->regmap, REG_CURRENT, &regval);
        if (ret)
            return ret;

        /* Convert to milliamps */
        *val = (s16)regval * pm->current_lsb / 1000;
        return 0;

    case hwmon_power:
        if (channel != 0 || attr != hwmon_power_input)
            return -EOPNOTSUPP;

        ret = regmap_read(pm->regmap, REG_POWER, &regval);
        if (ret)
            return ret;

        /* Power in microwatts */
        *val = regval * pm->voltage_lsb * pm->current_lsb / 1000;
        return 0;

    default:
        return -EOPNOTSUPP;
    }
}

static umode_t power_monitor_is_visible(const void *data,
                                        enum hwmon_sensor_types type,
                                        u32 attr, int channel)
{
    if (channel != 0)
        return 0;

    switch (type) {
    case hwmon_in:
        return (attr == hwmon_in_input) ? 0444 : 0;
    case hwmon_curr:
        return (attr == hwmon_curr_input) ? 0444 : 0;
    case hwmon_power:
        return (attr == hwmon_power_input) ? 0444 : 0;
    default:
        return 0;
    }
}

static const struct hwmon_channel_info * const power_monitor_info[] = {
    HWMON_CHANNEL_INFO(in, HWMON_I_INPUT),
    HWMON_CHANNEL_INFO(curr, HWMON_C_INPUT),
    HWMON_CHANNEL_INFO(power, HWMON_P_INPUT),
    NULL
};

static const struct hwmon_ops power_monitor_ops = {
    .is_visible = power_monitor_is_visible,
    .read = power_monitor_read,
};

static const struct hwmon_chip_info power_monitor_chip_info = {
    .ops = &power_monitor_ops,
    .info = power_monitor_info,
};

static int power_monitor_probe(struct i2c_client *client)
{
    struct power_monitor *pm;
    struct device *hwmon_dev;
    u32 shunt;

    pm = devm_kzalloc(&client->dev, sizeof(*pm), GFP_KERNEL);
    if (!pm)
        return -ENOMEM;

    /* Read shunt resistor from device tree */
    if (device_property_read_u32(&client->dev,
                                 "shunt-resistor-micro-ohms", &shunt))
        shunt = 10000;  /* Default 10 mΩ */

    pm->shunt_resistor = shunt;
    pm->voltage_lsb = 1250;  /* 1.25mV per LSB (example) */
    pm->current_lsb = 100;   /* 100µA per LSB (example) */

    pm->regmap = devm_regmap_init_i2c(client, &power_regmap_config);
    if (IS_ERR(pm->regmap))
        return PTR_ERR(pm->regmap);

    hwmon_dev = devm_hwmon_device_register_with_info(&client->dev,
                                                     "power_monitor",
                                                     pm,
                                                     &power_monitor_chip_info,
                                                     NULL);
    return PTR_ERR_OR_ZERO(hwmon_dev);
}
```

## Extra sysfs Attributes

Add custom attributes alongside HWMON:

```c
static ssize_t serial_number_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    struct my_hwmon *hwmon = dev_get_drvdata(dev);
    u32 serial = readl(hwmon->regs + REG_SERIAL);
    return sysfs_emit(buf, "%08X\n", serial);
}
static DEVICE_ATTR_RO(serial_number);

static struct attribute *my_hwmon_attrs[] = {
    &dev_attr_serial_number.attr,
    NULL
};

ATTRIBUTE_GROUPS(my_hwmon);

/* In probe */
hwmon_dev = devm_hwmon_device_register_with_info(&pdev->dev,
                                                 "my_hwmon",
                                                 hwmon,
                                                 &my_hwmon_chip_info,
                                                 my_hwmon_groups);  /* Extra groups */
```

## Device Tree Binding

```dts
temp_sensor: sensor@48 {
    compatible = "vendor,temp-sensor";
    reg = <0x48>;
    #thermal-sensor-cells = <0>;
};

power_monitor: power@40 {
    compatible = "vendor,power-monitor";
    reg = <0x40>;
    shunt-resistor-micro-ohms = <10000>;  /* 10mΩ */
};
```

## Summary

- Use `HWMON_CHANNEL_INFO()` macro for channel definition
- Implement `read()`, `write()`, `is_visible()` callbacks
- Return values in standard units (millidegrees, millivolts, etc.)
- Use `read_string()` for sensor labels
- `devm_hwmon_device_register_with_info()` for managed registration
- Extra attribute groups for custom sysfs entries

## Further Reading

- [HWMON Kernel API](https://docs.kernel.org/hwmon/hwmon-kernel-api.html) - API documentation
- [HWMON sysfs Interface](https://docs.kernel.org/hwmon/sysfs-interface.html) - Attribute naming
- [Example Drivers](https://elixir.bootlin.com/linux/v6.6/source/drivers/hwmon/tmp421.c) - tmp421.c

## Next

Learn about the [LED subsystem]({% link part10/07-led-subsystem.md %}).
