// SPDX-License-Identifier: GPL-2.0
/*
 * HWMON Demo Driver
 *
 * Demonstrates implementing a hardware monitoring driver with
 * temperature, voltage, and fan speed sensors.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/random.h>

#define DRIVER_NAME     "demo-hwmon"
#define NUM_TEMP        2
#define NUM_VOLTAGE     2
#define NUM_FAN         1

struct demo_hwmon {
    struct device *hwmon_dev;

    /* Simulated sensor values */
    long temp[NUM_TEMP];          /* millidegrees */
    long temp_max[NUM_TEMP];
    long temp_crit[NUM_TEMP];

    long voltage[NUM_VOLTAGE];    /* millivolts */
    long voltage_min[NUM_VOLTAGE];
    long voltage_max[NUM_VOLTAGE];

    long fan_rpm[NUM_FAN];
    long fan_min[NUM_FAN];

    /* Labels */
    const char *temp_labels[NUM_TEMP];
    const char *voltage_labels[NUM_VOLTAGE];
    const char *fan_labels[NUM_FAN];
};

/* Simulate reading temperature with some variation */
static long simulate_temp(struct demo_hwmon *hwmon, int channel)
{
    long base = hwmon->temp[channel];
    long variation;

    get_random_bytes(&variation, sizeof(variation));
    variation = (variation % 2000) - 1000;  /* ±1°C variation */

    return base + variation;
}

/* Simulate fan speed */
static long simulate_fan(struct demo_hwmon *hwmon, int channel)
{
    long base = hwmon->fan_rpm[channel];
    long variation;

    get_random_bytes(&variation, sizeof(variation));
    variation = (variation % 200) - 100;  /* ±100 RPM variation */

    return max(0L, base + variation);
}

static int demo_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
                           u32 attr, int channel, long *val)
{
    struct demo_hwmon *hwmon = dev_get_drvdata(dev);

    switch (type) {
    case hwmon_temp:
        switch (attr) {
        case hwmon_temp_input:
            *val = simulate_temp(hwmon, channel);
            return 0;
        case hwmon_temp_max:
            *val = hwmon->temp_max[channel];
            return 0;
        case hwmon_temp_crit:
            *val = hwmon->temp_crit[channel];
            return 0;
        case hwmon_temp_max_alarm:
            *val = simulate_temp(hwmon, channel) > hwmon->temp_max[channel];
            return 0;
        case hwmon_temp_crit_alarm:
            *val = simulate_temp(hwmon, channel) > hwmon->temp_crit[channel];
            return 0;
        }
        break;

    case hwmon_in:
        switch (attr) {
        case hwmon_in_input:
            *val = hwmon->voltage[channel];
            return 0;
        case hwmon_in_min:
            *val = hwmon->voltage_min[channel];
            return 0;
        case hwmon_in_max:
            *val = hwmon->voltage_max[channel];
            return 0;
        }
        break;

    case hwmon_fan:
        switch (attr) {
        case hwmon_fan_input:
            *val = simulate_fan(hwmon, channel);
            return 0;
        case hwmon_fan_min:
            *val = hwmon->fan_min[channel];
            return 0;
        case hwmon_fan_alarm:
            *val = simulate_fan(hwmon, channel) < hwmon->fan_min[channel];
            return 0;
        }
        break;

    default:
        break;
    }

    return -EOPNOTSUPP;
}

static int demo_hwmon_write(struct device *dev, enum hwmon_sensor_types type,
                            u32 attr, int channel, long val)
{
    struct demo_hwmon *hwmon = dev_get_drvdata(dev);

    switch (type) {
    case hwmon_temp:
        switch (attr) {
        case hwmon_temp_max:
            hwmon->temp_max[channel] = val;
            return 0;
        case hwmon_temp_crit:
            hwmon->temp_crit[channel] = val;
            return 0;
        }
        break;

    case hwmon_in:
        switch (attr) {
        case hwmon_in_min:
            hwmon->voltage_min[channel] = val;
            return 0;
        case hwmon_in_max:
            hwmon->voltage_max[channel] = val;
            return 0;
        }
        break;

    case hwmon_fan:
        switch (attr) {
        case hwmon_fan_min:
            hwmon->fan_min[channel] = val;
            return 0;
        }
        break;

    default:
        break;
    }

    return -EOPNOTSUPP;
}

static int demo_hwmon_read_string(struct device *dev,
                                  enum hwmon_sensor_types type,
                                  u32 attr, int channel, const char **str)
{
    struct demo_hwmon *hwmon = dev_get_drvdata(dev);

    switch (type) {
    case hwmon_temp:
        if (attr == hwmon_temp_label && channel < NUM_TEMP) {
            *str = hwmon->temp_labels[channel];
            return 0;
        }
        break;

    case hwmon_in:
        if (attr == hwmon_in_label && channel < NUM_VOLTAGE) {
            *str = hwmon->voltage_labels[channel];
            return 0;
        }
        break;

    case hwmon_fan:
        if (attr == hwmon_fan_label && channel < NUM_FAN) {
            *str = hwmon->fan_labels[channel];
            return 0;
        }
        break;

    default:
        break;
    }

    return -EOPNOTSUPP;
}

static umode_t demo_hwmon_is_visible(const void *data,
                                     enum hwmon_sensor_types type,
                                     u32 attr, int channel)
{
    switch (type) {
    case hwmon_temp:
        if (channel >= NUM_TEMP)
            return 0;
        switch (attr) {
        case hwmon_temp_input:
        case hwmon_temp_label:
        case hwmon_temp_max_alarm:
        case hwmon_temp_crit_alarm:
            return 0444;
        case hwmon_temp_max:
        case hwmon_temp_crit:
            return 0644;
        }
        break;

    case hwmon_in:
        if (channel >= NUM_VOLTAGE)
            return 0;
        switch (attr) {
        case hwmon_in_input:
        case hwmon_in_label:
            return 0444;
        case hwmon_in_min:
        case hwmon_in_max:
            return 0644;
        }
        break;

    case hwmon_fan:
        if (channel >= NUM_FAN)
            return 0;
        switch (attr) {
        case hwmon_fan_input:
        case hwmon_fan_label:
        case hwmon_fan_alarm:
            return 0444;
        case hwmon_fan_min:
            return 0644;
        }
        break;

    default:
        break;
    }

    return 0;
}

static const struct hwmon_channel_info * const demo_hwmon_info[] = {
    HWMON_CHANNEL_INFO(temp,
                       HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT |
                       HWMON_T_LABEL | HWMON_T_MAX_ALARM | HWMON_T_CRIT_ALARM,
                       HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT |
                       HWMON_T_LABEL | HWMON_T_MAX_ALARM | HWMON_T_CRIT_ALARM),
    HWMON_CHANNEL_INFO(in,
                       HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX | HWMON_I_LABEL,
                       HWMON_I_INPUT | HWMON_I_MIN | HWMON_I_MAX | HWMON_I_LABEL),
    HWMON_CHANNEL_INFO(fan,
                       HWMON_F_INPUT | HWMON_F_MIN | HWMON_F_LABEL | HWMON_F_ALARM),
    NULL
};

static const struct hwmon_ops demo_hwmon_ops = {
    .is_visible = demo_hwmon_is_visible,
    .read = demo_hwmon_read,
    .write = demo_hwmon_write,
    .read_string = demo_hwmon_read_string,
};

static const struct hwmon_chip_info demo_hwmon_chip_info = {
    .ops = &demo_hwmon_ops,
    .info = demo_hwmon_info,
};

static int demo_hwmon_probe(struct platform_device *pdev)
{
    struct demo_hwmon *hwmon;
    struct device *hwmon_dev;

    hwmon = devm_kzalloc(&pdev->dev, sizeof(*hwmon), GFP_KERNEL);
    if (!hwmon)
        return -ENOMEM;

    /* Initialize simulated sensor values */
    hwmon->temp[0] = 45000;          /* 45°C */
    hwmon->temp[1] = 55000;          /* 55°C */
    hwmon->temp_max[0] = 80000;      /* 80°C max */
    hwmon->temp_max[1] = 85000;
    hwmon->temp_crit[0] = 100000;    /* 100°C critical */
    hwmon->temp_crit[1] = 105000;

    hwmon->voltage[0] = 3300;        /* 3.3V */
    hwmon->voltage[1] = 5000;        /* 5.0V */
    hwmon->voltage_min[0] = 3135;    /* 3.3V -5% */
    hwmon->voltage_max[0] = 3465;    /* 3.3V +5% */
    hwmon->voltage_min[1] = 4750;    /* 5.0V -5% */
    hwmon->voltage_max[1] = 5250;    /* 5.0V +5% */

    hwmon->fan_rpm[0] = 2500;        /* 2500 RPM */
    hwmon->fan_min[0] = 1000;        /* 1000 RPM minimum */

    /* Sensor labels */
    hwmon->temp_labels[0] = "CPU";
    hwmon->temp_labels[1] = "Ambient";
    hwmon->voltage_labels[0] = "3.3V Rail";
    hwmon->voltage_labels[1] = "5V Rail";
    hwmon->fan_labels[0] = "System Fan";

    hwmon_dev = devm_hwmon_device_register_with_info(&pdev->dev,
                                                     "demo_hwmon",
                                                     hwmon,
                                                     &demo_hwmon_chip_info,
                                                     NULL);
    if (IS_ERR(hwmon_dev))
        return PTR_ERR(hwmon_dev);

    hwmon->hwmon_dev = hwmon_dev;
    platform_set_drvdata(pdev, hwmon);

    dev_info(&pdev->dev, "HWMON demo registered: %d temp, %d voltage, %d fan\n",
             NUM_TEMP, NUM_VOLTAGE, NUM_FAN);

    return 0;
}

static const struct of_device_id demo_hwmon_of_match[] = {
    { .compatible = "demo,hwmon" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_hwmon_of_match);

static struct platform_driver demo_hwmon_driver = {
    .probe = demo_hwmon_probe,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = demo_hwmon_of_match,
    },
};

/* Self-registering platform device */
static struct platform_device *demo_pdev;

static int __init demo_hwmon_init(void)
{
    int ret;

    ret = platform_driver_register(&demo_hwmon_driver);
    if (ret)
        return ret;

    demo_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(demo_pdev)) {
        platform_driver_unregister(&demo_hwmon_driver);
        return PTR_ERR(demo_pdev);
    }

    return 0;
}

static void __exit demo_hwmon_exit(void)
{
    platform_device_unregister(demo_pdev);
    platform_driver_unregister(&demo_hwmon_driver);
}

module_init(demo_hwmon_init);
module_exit(demo_hwmon_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Demo HWMON Driver");
