---
layout: default
title: "11.7 Regulator Driver"
parent: "Part 11: IIO, RTC, Regulator, Clock Drivers"
nav_order: 7
---

# Regulator Driver

This chapter covers implementing regulator provider drivers for PMICs and power management ICs.

## Key Structures

### regulator_desc

```c
#include <linux/regulator/driver.h>

struct regulator_desc {
    const char *name;               /* Regulator name */
    const char *supply_name;        /* Parent supply name */
    const char *of_match;           /* DT node name to match */
    const char *regulators_node;    /* DT node containing regulators */
    int id;                         /* Regulator ID */
    unsigned int n_voltages;        /* Number of voltage steps */
    const struct regulator_ops *ops;/* Operations */
    int irq;                        /* Interrupt (optional) */
    enum regulator_type type;       /* REGULATOR_VOLTAGE or _CURRENT */

    /* Voltage control */
    unsigned int min_uV;            /* Minimum voltage */
    unsigned int uV_step;           /* Voltage step */
    unsigned int linear_min_sel;    /* Min selector for linear range */
    const struct linear_range *linear_ranges;
    int n_linear_ranges;

    /* Register-based control */
    unsigned int vsel_reg;          /* Voltage selector register */
    unsigned int vsel_mask;         /* Voltage selector mask */
    unsigned int apply_reg;         /* Apply register */
    unsigned int apply_bit;         /* Apply bit */
    unsigned int enable_reg;        /* Enable register */
    unsigned int enable_mask;       /* Enable mask */
    unsigned int enable_val;        /* Value to enable */
    unsigned int disable_val;       /* Value to disable */
    bool enable_is_inverted;        /* Invert enable logic */

    /* Timing */
    unsigned int enable_time;       /* Time to enable (µs) */
    unsigned int off_on_delay;      /* Off-to-on delay (µs) */
    unsigned int ramp_delay;        /* Ramp rate (µV/µs) */
    /* ... */
};
```

### regulator_ops

```c
struct regulator_ops {
    /* Voltage operations */
    int (*list_voltage)(struct regulator_dev *rdev, unsigned int selector);
    int (*set_voltage)(struct regulator_dev *rdev, int min_uV, int max_uV,
                       unsigned int *selector);
    int (*set_voltage_sel)(struct regulator_dev *rdev, unsigned int selector);
    int (*get_voltage)(struct regulator_dev *rdev);
    int (*get_voltage_sel)(struct regulator_dev *rdev);

    /* Enable/disable */
    int (*enable)(struct regulator_dev *rdev);
    int (*disable)(struct regulator_dev *rdev);
    int (*is_enabled)(struct regulator_dev *rdev);

    /* Current limit */
    int (*set_current_limit)(struct regulator_dev *rdev, int min_uA, int max_uA);
    int (*get_current_limit)(struct regulator_dev *rdev);

    /* Mode control */
    int (*set_mode)(struct regulator_dev *rdev, unsigned int mode);
    unsigned int (*get_mode)(struct regulator_dev *rdev);

    /* Power management */
    int (*set_suspend_voltage)(struct regulator_dev *rdev, int uV);
    int (*set_suspend_enable)(struct regulator_dev *rdev);
    int (*set_suspend_disable)(struct regulator_dev *rdev);
    int (*set_suspend_mode)(struct regulator_dev *rdev, unsigned int mode);
    /* ... */
};
```

## Simple LDO Driver

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>

#define REG_LDO1_CTRL   0x10
#define REG_LDO1_VSEL   0x11
#define REG_LDO2_CTRL   0x12
#define REG_LDO2_VSEL   0x13

#define LDO_EN          BIT(7)
#define LDO_VSEL_MASK   0x1F

#define NUM_REGULATORS  2

struct my_pmic {
    struct regmap *regmap;
    struct regulator_dev *regulators[NUM_REGULATORS];
};

/* Voltage table: 1.8V to 3.3V in 100mV steps */
static const unsigned int ldo_voltages[] = {
    1800000, 1900000, 2000000, 2100000, 2200000, 2300000, 2400000, 2500000,
    2600000, 2700000, 2800000, 2900000, 3000000, 3100000, 3200000, 3300000,
};

static const struct regulator_ops my_ldo_ops = {
    .list_voltage = regulator_list_voltage_table,
    .map_voltage = regulator_map_voltage_ascend,
    .set_voltage_sel = regulator_set_voltage_sel_regmap,
    .get_voltage_sel = regulator_get_voltage_sel_regmap,
    .enable = regulator_enable_regmap,
    .disable = regulator_disable_regmap,
    .is_enabled = regulator_is_enabled_regmap,
};

static const struct regulator_desc my_regulators[] = {
    {
        .name = "LDO1",
        .of_match = "ldo1",
        .id = 0,
        .type = REGULATOR_VOLTAGE,
        .owner = THIS_MODULE,
        .ops = &my_ldo_ops,
        .volt_table = ldo_voltages,
        .n_voltages = ARRAY_SIZE(ldo_voltages),
        .vsel_reg = REG_LDO1_VSEL,
        .vsel_mask = LDO_VSEL_MASK,
        .enable_reg = REG_LDO1_CTRL,
        .enable_mask = LDO_EN,
        .enable_val = LDO_EN,
        .disable_val = 0,
        .enable_time = 200,  /* 200µs */
    },
    {
        .name = "LDO2",
        .of_match = "ldo2",
        .id = 1,
        .type = REGULATOR_VOLTAGE,
        .owner = THIS_MODULE,
        .ops = &my_ldo_ops,
        .volt_table = ldo_voltages,
        .n_voltages = ARRAY_SIZE(ldo_voltages),
        .vsel_reg = REG_LDO2_VSEL,
        .vsel_mask = LDO_VSEL_MASK,
        .enable_reg = REG_LDO2_CTRL,
        .enable_mask = LDO_EN,
        .enable_val = LDO_EN,
        .disable_val = 0,
        .enable_time = 200,
    },
};

static const struct regmap_config my_pmic_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x1F,
};

static int my_pmic_probe(struct i2c_client *client)
{
    struct my_pmic *pmic;
    struct regulator_config config = {};
    int i, ret;

    pmic = devm_kzalloc(&client->dev, sizeof(*pmic), GFP_KERNEL);
    if (!pmic)
        return -ENOMEM;

    pmic->regmap = devm_regmap_init_i2c(client, &my_pmic_regmap_config);
    if (IS_ERR(pmic->regmap))
        return PTR_ERR(pmic->regmap);

    config.dev = &client->dev;
    config.regmap = pmic->regmap;

    for (i = 0; i < NUM_REGULATORS; i++) {
        config.driver_data = pmic;

        pmic->regulators[i] = devm_regulator_register(&client->dev,
                                                      &my_regulators[i],
                                                      &config);
        if (IS_ERR(pmic->regulators[i])) {
            ret = PTR_ERR(pmic->regulators[i]);
            return dev_err_probe(&client->dev, ret,
                                 "Failed to register %s\n",
                                 my_regulators[i].name);
        }
    }

    i2c_set_clientdata(client, pmic);

    dev_info(&client->dev, "Registered %d regulators\n", NUM_REGULATORS);
    return 0;
}

static const struct of_device_id my_pmic_of_match[] = {
    { .compatible = "vendor,my-pmic" },
    { }
};
MODULE_DEVICE_TABLE(of, my_pmic_of_match);

static struct i2c_driver my_pmic_driver = {
    .driver = {
        .name = "my-pmic",
        .of_match_table = my_pmic_of_match,
    },
    .probe = my_pmic_probe,
};
module_i2c_driver(my_pmic_driver);

MODULE_LICENSE("GPL");
```

## Linear Range Regulator

```c
/* For regulators with linear voltage steps */
static const struct linear_range buck_ranges[] = {
    REGULATOR_LINEAR_RANGE(500000, 0, 63, 12500),    /* 0.5V-1.3V, 12.5mV step */
    REGULATOR_LINEAR_RANGE(1300000, 64, 127, 25000), /* 1.3V-2.9V, 25mV step */
};

static const struct regulator_desc buck_desc = {
    .name = "BUCK1",
    .of_match = "buck1",
    .id = 0,
    .type = REGULATOR_VOLTAGE,
    .owner = THIS_MODULE,
    .ops = &my_buck_ops,
    .linear_ranges = buck_ranges,
    .n_linear_ranges = ARRAY_SIZE(buck_ranges),
    .n_voltages = 128,
    .vsel_reg = REG_BUCK1_VSEL,
    .vsel_mask = 0x7F,
    .enable_reg = REG_BUCK1_CTRL,
    .enable_mask = BIT(0),
    .ramp_delay = 6250,  /* 6.25mV/µs */
};

static const struct regulator_ops my_buck_ops = {
    .list_voltage = regulator_list_voltage_linear_range,
    .map_voltage = regulator_map_voltage_linear_range,
    .set_voltage_sel = regulator_set_voltage_sel_regmap,
    .get_voltage_sel = regulator_get_voltage_sel_regmap,
    .enable = regulator_enable_regmap,
    .disable = regulator_disable_regmap,
    .is_enabled = regulator_is_enabled_regmap,
    .set_ramp_delay = regulator_set_ramp_delay_regmap,
};
```

## Custom Operations

```c
static int my_ldo_set_voltage(struct regulator_dev *rdev,
                              int min_uV, int max_uV,
                              unsigned int *selector)
{
    struct my_pmic *pmic = rdev_get_drvdata(rdev);
    int sel;

    /* Find best selector */
    for (sel = 0; sel < rdev->desc->n_voltages; sel++) {
        int voltage = ldo_voltages[sel];
        if (voltage >= min_uV && voltage <= max_uV) {
            *selector = sel;

            /* Write to hardware */
            return regmap_update_bits(pmic->regmap,
                                      rdev->desc->vsel_reg,
                                      rdev->desc->vsel_mask,
                                      sel);
        }
    }

    return -EINVAL;
}

static int my_ldo_get_voltage(struct regulator_dev *rdev)
{
    struct my_pmic *pmic = rdev_get_drvdata(rdev);
    unsigned int val;
    int ret;

    ret = regmap_read(pmic->regmap, rdev->desc->vsel_reg, &val);
    if (ret)
        return ret;

    val &= rdev->desc->vsel_mask;

    if (val >= rdev->desc->n_voltages)
        return -EINVAL;

    return ldo_voltages[val];
}
```

## Device Tree Binding

```dts
&i2c1 {
    pmic@48 {
        compatible = "vendor,my-pmic";
        reg = <0x48>;

        regulators {
            ldo1: ldo1 {
                regulator-name = "vdd-sensor";
                regulator-min-microvolt = <1800000>;
                regulator-max-microvolt = <3300000>;
            };

            ldo2: ldo2 {
                regulator-name = "vdd-io";
                regulator-min-microvolt = <1800000>;
                regulator-max-microvolt = <3300000>;
                regulator-boot-on;
            };
        };
    };
};
```

## Summary

- `regulator_desc` defines regulator properties
- Use regmap helpers for register-based control
- Support voltage tables or linear ranges
- `devm_regulator_register()` for managed registration
- Device Tree defines constraints per regulator

## Further Reading

- [Regulator Driver API](https://docs.kernel.org/power/regulator/design.html) - Design documentation
- [Regulator Drivers](https://elixir.bootlin.com/linux/v6.6/source/drivers/regulator) - Examples
- [AXP20x Driver](https://elixir.bootlin.com/linux/v6.6/source/drivers/regulator/axp20x-regulator.c) - Reference

## Next

Learn about the [Clock Framework]({% link part11/08-clock-framework.md %}).
