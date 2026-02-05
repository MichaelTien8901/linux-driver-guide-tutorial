---
layout: default
title: "8.5 Clocks and Regulators"
parent: "Part 8: Platform Bus and Device Tree"
nav_order: 5
---

# Clocks and Regulators

Most devices need clocks and power supplies. Device Tree describes these dependencies, and drivers use kernel frameworks to manage them.

## Clocks in Device Tree

### Clock Provider

```dts
/* Fixed clock */
osc: oscillator {
    compatible = "fixed-clock";
    #clock-cells = <0>;
    clock-frequency = <24000000>;  /* 24 MHz */
};

/* Clock controller with multiple outputs */
clk: clock-controller@10000000 {
    compatible = "vendor,clock-controller";
    reg = <0x10000000 0x1000>;
    #clock-cells = <1>;
    /* Provides multiple clocks: 0=core, 1=bus, 2=uart, etc. */
};
```

### Clock Consumer

```dts
uart@20000000 {
    compatible = "vendor,uart";
    reg = <0x20000000 0x1000>;

    /* Single clock reference */
    clocks = <&clk 2>;  /* Clock output 2 from clk controller */
    clock-names = "uart";

    /* Multiple clocks */
    clocks = <&clk 0>, <&osc>;
    clock-names = "core", "baud";
};
```

## Using Clocks in Driver

```c
#include <linux/clk.h>

struct my_device {
    struct clk *core_clk;
    struct clk *baud_clk;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_device *dev;
    int ret;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* Get clock by name */
    dev->core_clk = devm_clk_get(&pdev->dev, "core");
    if (IS_ERR(dev->core_clk))
        return dev_err_probe(&pdev->dev, PTR_ERR(dev->core_clk),
                             "Failed to get core clock\n");

    /* Optional clock */
    dev->baud_clk = devm_clk_get_optional(&pdev->dev, "baud");
    if (IS_ERR(dev->baud_clk))
        return PTR_ERR(dev->baud_clk);

    /* Enable clocks */
    ret = clk_prepare_enable(dev->core_clk);
    if (ret)
        return ret;

    /* Register cleanup action */
    ret = devm_add_action_or_reset(&pdev->dev,
                (void (*)(void *))clk_disable_unprepare, dev->core_clk);
    if (ret)
        return ret;

    /* Get clock rate */
    unsigned long rate = clk_get_rate(dev->core_clk);
    dev_info(&pdev->dev, "Core clock rate: %lu Hz\n", rate);

    return 0;
}
```

### Clock Operations

```c
/* Prepare and enable (two-stage) */
clk_prepare(clk);           /* May sleep, do in probe */
clk_enable(clk);            /* Fast, can do in IRQ */

/* Combined (most common) */
clk_prepare_enable(clk);    /* Prepare + enable */
clk_disable_unprepare(clk); /* Disable + unprepare */

/* Get/set rate */
unsigned long rate = clk_get_rate(clk);
int ret = clk_set_rate(clk, 48000000);

/* Round rate to nearest supported */
long new_rate = clk_round_rate(clk, 48000000);

/* Get parent clock */
struct clk *parent = clk_get_parent(clk);
```

### Convenience Functions

```c
/* Get clock already prepared */
dev->clk = devm_clk_get_prepared(&pdev->dev, "core");

/* Get clock already enabled */
dev->clk = devm_clk_get_enabled(&pdev->dev, "core");

/* Optional + enabled */
dev->clk = devm_clk_get_optional_enabled(&pdev->dev, "optional");
```

## Regulators in Device Tree

### Regulator Provider (PMIC)

```dts
pmic: pmic@48 {
    compatible = "vendor,pmic";
    reg = <0x48>;

    regulators {
        vdd_core: buck1 {
            regulator-name = "vdd-core";
            regulator-min-microvolt = <900000>;
            regulator-max-microvolt = <1200000>;
            regulator-boot-on;
        };

        vdd_io: ldo1 {
            regulator-name = "vdd-io";
            regulator-min-microvolt = <1800000>;
            regulator-max-microvolt = <3300000>;
        };

        vdd_3v3: ldo2 {
            regulator-name = "vdd-3v3";
            regulator-min-microvolt = <3300000>;
            regulator-max-microvolt = <3300000>;
            regulator-always-on;
        };
    };
};
```

### Regulator Consumer

```dts
sensor@50 {
    compatible = "vendor,sensor";
    reg = <0x50>;

    /* Single supply */
    vdd-supply = <&vdd_io>;

    /* Multiple supplies */
    vdd-supply = <&vdd_core>;
    vddio-supply = <&vdd_io>;
};
```

## Using Regulators in Driver

```c
#include <linux/regulator/consumer.h>

struct my_device {
    struct regulator *vdd;
    struct regulator *vddio;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_device *dev;
    int ret;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* Get regulator */
    dev->vdd = devm_regulator_get(&pdev->dev, "vdd");
    if (IS_ERR(dev->vdd))
        return dev_err_probe(&pdev->dev, PTR_ERR(dev->vdd),
                             "Failed to get vdd regulator\n");

    /* Optional regulator */
    dev->vddio = devm_regulator_get_optional(&pdev->dev, "vddio");
    if (IS_ERR(dev->vddio)) {
        if (PTR_ERR(dev->vddio) == -ENODEV)
            dev->vddio = NULL;  /* Not present, okay */
        else
            return PTR_ERR(dev->vddio);
    }

    /* Enable regulator */
    ret = regulator_enable(dev->vdd);
    if (ret)
        return ret;

    /* Register cleanup */
    ret = devm_add_action_or_reset(&pdev->dev,
                (void (*)(void *))regulator_disable, dev->vdd);
    if (ret)
        return ret;

    return 0;
}
```

### Regulator Operations

```c
/* Enable/disable */
regulator_enable(reg);
regulator_disable(reg);

/* Check if enabled */
int enabled = regulator_is_enabled(reg);

/* Set voltage */
regulator_set_voltage(reg, min_uV, max_uV);

/* Get current voltage */
int uV = regulator_get_voltage(reg);

/* Set load (for efficiency optimization) */
regulator_set_load(reg, load_uA);
```

### Bulk Regulator API

```c
struct regulator_bulk_data supplies[] = {
    { .supply = "vdd" },
    { .supply = "vddio" },
    { .supply = "vref" },
};

/* Get all regulators */
ret = devm_regulator_bulk_get(&pdev->dev, ARRAY_SIZE(supplies), supplies);
if (ret)
    return ret;

/* Enable all */
ret = regulator_bulk_enable(ARRAY_SIZE(supplies), supplies);

/* Disable all */
regulator_bulk_disable(ARRAY_SIZE(supplies), supplies);
```

## Combined Clock and Regulator Example

```dts
sensor@50 {
    compatible = "vendor,temp-sensor";
    reg = <0x50>;

    /* Power supply */
    vdd-supply = <&vdd_sensor>;

    /* Clock input */
    clocks = <&clk_sensor>;
    clock-names = "sensor";

    /* Interrupt */
    interrupt-parent = <&gpio>;
    interrupts = <10 IRQ_TYPE_EDGE_FALLING>;
};
```

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>

struct sensor_device {
    struct i2c_client *client;
    struct regulator *vdd;
    struct clk *clk;
};

static int sensor_power_on(struct sensor_device *sensor)
{
    int ret;

    /* Enable power first */
    ret = regulator_enable(sensor->vdd);
    if (ret) {
        dev_err(&sensor->client->dev, "Failed to enable vdd\n");
        return ret;
    }

    /* Wait for power to stabilize */
    usleep_range(1000, 2000);

    /* Then enable clock */
    ret = clk_prepare_enable(sensor->clk);
    if (ret) {
        dev_err(&sensor->client->dev, "Failed to enable clock\n");
        regulator_disable(sensor->vdd);
        return ret;
    }

    /* Wait for clock to stabilize */
    usleep_range(100, 200);

    return 0;
}

static void sensor_power_off(struct sensor_device *sensor)
{
    /* Disable clock first */
    clk_disable_unprepare(sensor->clk);

    /* Then disable power */
    regulator_disable(sensor->vdd);
}

static int sensor_probe(struct i2c_client *client)
{
    struct sensor_device *sensor;
    int ret;

    sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return -ENOMEM;

    sensor->client = client;

    /* Get regulator */
    sensor->vdd = devm_regulator_get(&client->dev, "vdd");
    if (IS_ERR(sensor->vdd))
        return dev_err_probe(&client->dev, PTR_ERR(sensor->vdd),
                             "Failed to get vdd\n");

    /* Get clock */
    sensor->clk = devm_clk_get(&client->dev, "sensor");
    if (IS_ERR(sensor->clk))
        return dev_err_probe(&client->dev, PTR_ERR(sensor->clk),
                             "Failed to get clock\n");

    /* Power on sequence */
    ret = sensor_power_on(sensor);
    if (ret)
        return ret;

    /* Read chip ID to verify communication */
    ret = sensor_read_chip_id(sensor);
    if (ret) {
        sensor_power_off(sensor);
        return ret;
    }

    i2c_set_clientdata(client, sensor);

    dev_info(&client->dev, "Sensor initialized\n");
    return 0;
}

static void sensor_remove(struct i2c_client *client)
{
    struct sensor_device *sensor = i2c_get_clientdata(client);

    sensor_power_off(sensor);
}

static const struct of_device_id sensor_of_match[] = {
    { .compatible = "vendor,temp-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, sensor_of_match);

static struct i2c_driver sensor_driver = {
    .probe = sensor_probe,
    .remove = sensor_remove,
    .driver = {
        .name = "temp-sensor",
        .of_match_table = sensor_of_match,
    },
};
module_i2c_driver(sensor_driver);
```

## Power Sequencing

Many devices require specific power-on/off sequences:

```c
static int device_power_on(struct my_device *dev)
{
    int ret;

    /* Step 1: Core voltage */
    ret = regulator_enable(dev->vdd_core);
    if (ret)
        return ret;
    usleep_range(100, 200);

    /* Step 2: I/O voltage */
    ret = regulator_enable(dev->vdd_io);
    if (ret)
        goto err_io;
    usleep_range(50, 100);

    /* Step 3: Enable clock */
    ret = clk_prepare_enable(dev->clk);
    if (ret)
        goto err_clk;

    /* Step 4: Release reset */
    gpiod_set_value_cansleep(dev->reset_gpio, 0);
    usleep_range(1000, 2000);

    return 0;

err_clk:
    regulator_disable(dev->vdd_io);
err_io:
    regulator_disable(dev->vdd_core);
    return ret;
}

static void device_power_off(struct my_device *dev)
{
    /* Reverse order */
    gpiod_set_value_cansleep(dev->reset_gpio, 1);
    clk_disable_unprepare(dev->clk);
    regulator_disable(dev->vdd_io);
    regulator_disable(dev->vdd_core);
}
```

## Summary

### Clocks
- Use `clocks` and `clock-names` properties
- `devm_clk_get()` / `devm_clk_get_optional()` in driver
- `clk_prepare_enable()` / `clk_disable_unprepare()` for control
- Use `devm_clk_get_enabled()` for simpler code

### Regulators
- Use `*-supply` properties (e.g., `vdd-supply`)
- `devm_regulator_get()` / `devm_regulator_get_optional()` in driver
- `regulator_enable()` / `regulator_disable()` for control
- Use bulk API for multiple supplies

### Sequencing
- Power supplies before clocks
- Clocks before releasing reset
- Respect timing requirements from datasheet

## Next

Learn about [Device Tree overlays]({% link part8/06-overlays.md %}) for runtime hardware configuration.
