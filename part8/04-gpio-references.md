---
layout: default
title: "8.4 GPIO in Device Tree"
parent: "Part 8: Platform Bus and Device Tree"
nav_order: 4
---

# GPIO in Device Tree

GPIOs (General Purpose Input/Output) are commonly referenced in Device Tree to describe connections between devices and GPIO controllers.

## GPIO Properties in Device Tree

### Basic GPIO Reference

```dts
/* GPIO controller */
gpio0: gpio@10000000 {
    compatible = "vendor,gpio";
    gpio-controller;
    #gpio-cells = <2>;
    reg = <0x10000000 0x1000>;
};

/* Device using GPIO */
device@20000000 {
    compatible = "vendor,my-device";
    reg = <0x20000000 0x1000>;

    /* Named GPIO with flags */
    reset-gpios = <&gpio0 5 GPIO_ACTIVE_LOW>;
    enable-gpios = <&gpio0 6 GPIO_ACTIVE_HIGH>;
};
```

### GPIO Cell Format

```dts
/* #gpio-cells = <2> means: <&controller gpio_num flags> */
reset-gpios = <&gpio0 5 GPIO_ACTIVE_LOW>;
/*             ^      ^  ^
 *             |      |  |_ flags
 *             |      |___ GPIO number (pin 5)
 *             |__________ phandle to controller
 */
```

### Common GPIO Flags

```c
/* From include/dt-bindings/gpio/gpio.h */
GPIO_ACTIVE_HIGH    0   /* Active high (normal) */
GPIO_ACTIVE_LOW     1   /* Active low (inverted) */
GPIO_OPEN_DRAIN     4   /* Open drain output */
GPIO_OPEN_SOURCE    8   /* Open source output */
GPIO_PULL_UP        16  /* Internal pull-up */
GPIO_PULL_DOWN      32  /* Internal pull-down */
```

## Requesting GPIOs in Driver

### Using gpiod API (Recommended)

```c
#include <linux/gpio/consumer.h>

struct my_device {
    struct gpio_desc *reset_gpio;
    struct gpio_desc *enable_gpio;
    struct gpio_desc *irq_gpio;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_device *dev;
    int ret;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* Get required GPIO */
    dev->reset_gpio = devm_gpiod_get(&pdev->dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(dev->reset_gpio))
        return dev_err_probe(&pdev->dev, PTR_ERR(dev->reset_gpio),
                             "Failed to get reset GPIO\n");

    /* Get optional GPIO */
    dev->enable_gpio = devm_gpiod_get_optional(&pdev->dev, "enable",
                                                GPIOD_OUT_LOW);
    if (IS_ERR(dev->enable_gpio))
        return PTR_ERR(dev->enable_gpio);

    /* Get GPIO for input (e.g., interrupt source) */
    dev->irq_gpio = devm_gpiod_get_optional(&pdev->dev, "irq", GPIOD_IN);
    if (IS_ERR(dev->irq_gpio))
        return PTR_ERR(dev->irq_gpio);

    return 0;
}
```

### GPIO Directions

```c
/* Output GPIOs */
GPIOD_OUT_LOW       /* Output, initially deasserted */
GPIOD_OUT_HIGH      /* Output, initially asserted */
GPIOD_OUT_LOW_OPEN_DRAIN   /* Open drain, initially low */
GPIOD_OUT_HIGH_OPEN_DRAIN  /* Open drain, initially high */

/* Input GPIOs */
GPIOD_IN            /* Input */

/* Bidirectional */
GPIOD_ASIS          /* Don't change direction */
```

## Using GPIOs

### Setting Output Value

```c
/* Set GPIO value (considers active-low flag automatically) */
gpiod_set_value(dev->reset_gpio, 1);  /* Assert (active) */
gpiod_set_value(dev->reset_gpio, 0);  /* Deassert (inactive) */

/* Sleeping context version */
gpiod_set_value_cansleep(dev->reset_gpio, 1);

/* Set multiple GPIOs atomically */
struct gpio_descs *gpios;
DECLARE_BITMAP(values, 4);

set_bit(0, values);
clear_bit(1, values);
gpiod_set_array_value(gpios->ndescs, gpios->desc, gpios->info, values);
```

### Reading Input Value

```c
/* Read GPIO value */
int value = gpiod_get_value(dev->irq_gpio);

/* Sleeping context version */
int value = gpiod_get_value_cansleep(dev->irq_gpio);
```

### Direction Changes

```c
/* Change direction */
gpiod_direction_output(gpio, 1);  /* Set as output, value 1 */
gpiod_direction_input(gpio);      /* Set as input */

/* Check direction */
if (gpiod_get_direction(gpio) == 0)
    pr_info("GPIO is output\n");
```

## Multiple GPIOs

### Array of GPIOs

```dts
device@10000000 {
    /* Multiple GPIOs with same function */
    data-gpios = <&gpio0 0 0>,
                 <&gpio0 1 0>,
                 <&gpio0 2 0>,
                 <&gpio0 3 0>;
};
```

```c
static int my_probe(struct platform_device *pdev)
{
    struct gpio_descs *data_gpios;
    int i;

    /* Get all GPIOs */
    data_gpios = devm_gpiod_get_array(&pdev->dev, "data", GPIOD_OUT_LOW);
    if (IS_ERR(data_gpios))
        return PTR_ERR(data_gpios);

    dev_info(&pdev->dev, "Got %d data GPIOs\n", data_gpios->ndescs);

    /* Access individual GPIOs */
    for (i = 0; i < data_gpios->ndescs; i++) {
        gpiod_set_value(data_gpios->desc[i], i & 1);
    }

    return 0;
}
```

### Indexed GPIOs

```dts
device@10000000 {
    /* Multiple reset GPIOs */
    reset-gpios = <&gpio0 5 GPIO_ACTIVE_LOW>,
                  <&gpio0 6 GPIO_ACTIVE_LOW>;
};
```

```c
/* Get specific GPIO by index */
struct gpio_desc *reset0, *reset1;

reset0 = devm_gpiod_get_index(&pdev->dev, "reset", 0, GPIOD_OUT_HIGH);
reset1 = devm_gpiod_get_index(&pdev->dev, "reset", 1, GPIOD_OUT_HIGH);

/* Optional version */
reset1 = devm_gpiod_get_index_optional(&pdev->dev, "reset", 1, GPIOD_OUT_HIGH);
```

## GPIO to IRQ

```dts
device@10000000 {
    irq-gpios = <&gpio0 10 GPIO_ACTIVE_LOW>;
};
```

```c
static int my_probe(struct platform_device *pdev)
{
    struct gpio_desc *irq_gpio;
    int irq;

    /* Get GPIO */
    irq_gpio = devm_gpiod_get(&pdev->dev, "irq", GPIOD_IN);
    if (IS_ERR(irq_gpio))
        return PTR_ERR(irq_gpio);

    /* Get IRQ number for this GPIO */
    irq = gpiod_to_irq(irq_gpio);
    if (irq < 0)
        return irq;

    /* Request IRQ */
    return devm_request_irq(&pdev->dev, irq, my_irq_handler,
                            IRQF_TRIGGER_FALLING,
                            "my-device", dev);
}
```

## Legacy GPIO API (Deprecated)

For reference in older code:

```c
#include <linux/gpio.h>  /* Legacy */

/* Old way - DO NOT USE in new code */
int gpio = of_get_named_gpio(np, "reset-gpios", 0);
gpio_request(gpio, "reset");
gpio_direction_output(gpio, 1);
gpio_set_value(gpio, 0);
gpio_free(gpio);

/* New way - USE THIS */
#include <linux/gpio/consumer.h>

struct gpio_desc *gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
gpiod_set_value(gpio, 0);
/* No need to free - devm handles it */
```

## GPIO Naming Conventions

```dts
/* Standard suffixed naming */
device@10000000 {
    /* These names map to driver's devm_gpiod_get(dev, "xxx", ...) */
    reset-gpios = <...>;       /* "reset" */
    enable-gpios = <...>;      /* "enable" */
    power-gpios = <...>;       /* "power" */
    cs-gpios = <...>;          /* "cs" (chip select) */
    wp-gpios = <...>;          /* "wp" (write protect) */
};
```

## Complete Example

```dts
/* Device Tree */
spi_device: device@0 {
    compatible = "vendor,my-spi-device";
    reg = <0>;
    spi-max-frequency = <1000000>;

    /* Control GPIOs */
    reset-gpios = <&gpio0 5 GPIO_ACTIVE_LOW>;
    enable-gpios = <&gpio0 6 GPIO_ACTIVE_HIGH>;

    /* Optional power control */
    power-gpios = <&gpio0 7 GPIO_ACTIVE_HIGH>;

    /* Interrupt */
    interrupt-parent = <&gpio0>;
    interrupts = <10 IRQ_TYPE_EDGE_FALLING>;
};
```

```c
/* Driver */
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>

struct my_device {
    struct spi_device *spi;
    struct gpio_desc *reset_gpio;
    struct gpio_desc *enable_gpio;
    struct gpio_desc *power_gpio;
};

static int my_hw_init(struct my_device *dev)
{
    /* Power on if power GPIO available */
    if (dev->power_gpio)
        gpiod_set_value_cansleep(dev->power_gpio, 1);

    /* Enable device */
    if (dev->enable_gpio) {
        gpiod_set_value_cansleep(dev->enable_gpio, 1);
        usleep_range(1000, 2000);  /* Enable settling time */
    }

    /* Reset sequence */
    gpiod_set_value_cansleep(dev->reset_gpio, 1);  /* Assert reset */
    usleep_range(100, 200);
    gpiod_set_value_cansleep(dev->reset_gpio, 0);  /* Release reset */
    usleep_range(1000, 2000);  /* Reset recovery time */

    return 0;
}

static int my_probe(struct spi_device *spi)
{
    struct my_device *dev;
    int ret;

    dev = devm_kzalloc(&spi->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->spi = spi;

    /* Get required reset GPIO */
    dev->reset_gpio = devm_gpiod_get(&spi->dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(dev->reset_gpio))
        return dev_err_probe(&spi->dev, PTR_ERR(dev->reset_gpio),
                             "Failed to get reset GPIO\n");

    /* Get optional GPIOs */
    dev->enable_gpio = devm_gpiod_get_optional(&spi->dev, "enable",
                                                GPIOD_OUT_LOW);
    if (IS_ERR(dev->enable_gpio))
        return PTR_ERR(dev->enable_gpio);

    dev->power_gpio = devm_gpiod_get_optional(&spi->dev, "power",
                                               GPIOD_OUT_LOW);
    if (IS_ERR(dev->power_gpio))
        return PTR_ERR(dev->power_gpio);

    /* Initialize hardware */
    ret = my_hw_init(dev);
    if (ret)
        return ret;

    spi_set_drvdata(spi, dev);
    return 0;
}

static void my_remove(struct spi_device *spi)
{
    struct my_device *dev = spi_get_drvdata(spi);

    /* Disable device */
    if (dev->enable_gpio)
        gpiod_set_value_cansleep(dev->enable_gpio, 0);

    /* Power off */
    if (dev->power_gpio)
        gpiod_set_value_cansleep(dev->power_gpio, 0);
}
```

## Summary

- Use `*-gpios` suffix in DT property names
- Use `devm_gpiod_get*()` functions for GPIO acquisition
- `_optional` variants for non-critical GPIOs
- `gpiod_set_value()` handles active-low logic automatically
- `*_cansleep` versions for I2C/SPI GPIO expanders
- Use `gpiod_to_irq()` to get IRQ number from GPIO

## Next

Learn about [clocks and regulators]({% link part8/05-clock-regulator-refs.md %}) in Device Tree.
