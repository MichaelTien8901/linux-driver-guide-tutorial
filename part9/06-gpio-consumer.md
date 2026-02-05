---
layout: default
title: "9.6 GPIO Consumer API"
parent: "Part 9: I2C, SPI, and GPIO Drivers"
nav_order: 6
---

# GPIO Consumer API

The GPIO consumer API allows drivers to request and use GPIO pins for various purposes like reset signals, enable pins, interrupts, and data lines.

## Getting GPIOs

### From Device Tree

```dts
device@10000000 {
    compatible = "vendor,my-device";
    reset-gpios = <&gpio0 5 GPIO_ACTIVE_LOW>;
    enable-gpios = <&gpio0 6 GPIO_ACTIVE_HIGH>;
    data-gpios = <&gpio0 7 0>,
                 <&gpio0 8 0>,
                 <&gpio0 9 0>,
                 <&gpio0 10 0>;
};
```

### In Driver Code

```c
#include <linux/gpio/consumer.h>

struct my_device {
    struct gpio_desc *reset_gpio;
    struct gpio_desc *enable_gpio;
    struct gpio_descs *data_gpios;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_device *dev;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* Required GPIO */
    dev->reset_gpio = devm_gpiod_get(&pdev->dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(dev->reset_gpio))
        return dev_err_probe(&pdev->dev, PTR_ERR(dev->reset_gpio),
                             "Failed to get reset GPIO\n");

    /* Optional GPIO */
    dev->enable_gpio = devm_gpiod_get_optional(&pdev->dev, "enable",
                                                GPIOD_OUT_LOW);
    if (IS_ERR(dev->enable_gpio))
        return PTR_ERR(dev->enable_gpio);

    /* Array of GPIOs */
    dev->data_gpios = devm_gpiod_get_array(&pdev->dev, "data", GPIOD_OUT_LOW);
    if (IS_ERR(dev->data_gpios))
        return PTR_ERR(dev->data_gpios);

    return 0;
}
```

## GPIO Acquisition Functions

```c
/* Single GPIO */
struct gpio_desc *devm_gpiod_get(struct device *dev,
                                  const char *con_id,
                                  enum gpiod_flags flags);

struct gpio_desc *devm_gpiod_get_optional(struct device *dev,
                                          const char *con_id,
                                          enum gpiod_flags flags);

/* By index (for multiple GPIOs with same name) */
struct gpio_desc *devm_gpiod_get_index(struct device *dev,
                                        const char *con_id,
                                        unsigned int idx,
                                        enum gpiod_flags flags);

/* Array of GPIOs */
struct gpio_descs *devm_gpiod_get_array(struct device *dev,
                                         const char *con_id,
                                         enum gpiod_flags flags);
```

## GPIO Flags

```c
/* Direction flags */
GPIOD_IN                     /* Input */
GPIOD_OUT_LOW                /* Output, initially deasserted */
GPIOD_OUT_HIGH               /* Output, initially asserted */
GPIOD_OUT_LOW_OPEN_DRAIN     /* Open drain, initially low */
GPIOD_OUT_HIGH_OPEN_DRAIN    /* Open drain, initially high */

/* For devm_gpiod_get_optional */
GPIOD_ASIS                   /* Don't change direction */
```

## Setting and Getting Values

### Output GPIOs

```c
/* Set logical value (handles active-low automatically) */
gpiod_set_value(gpio, 1);   /* Assert (active) */
gpiod_set_value(gpio, 0);   /* Deassert (inactive) */

/* Set with potential sleep (for I2C/SPI GPIO expanders) */
gpiod_set_value_cansleep(gpio, 1);

/* Set raw value (ignores active-low flag) */
gpiod_set_raw_value(gpio, 1);
```

### Input GPIOs

```c
/* Get logical value */
int value = gpiod_get_value(gpio);

/* Get with potential sleep */
int value = gpiod_get_value_cansleep(gpio);

/* Get raw value */
int value = gpiod_get_raw_value(gpio);
```

### Bulk Operations

```c
struct gpio_descs *gpios;
DECLARE_BITMAP(values, 8);

/* Set multiple GPIOs at once */
bitmap_zero(values, gpios->ndescs);
set_bit(0, values);  /* GPIO 0 = high */
set_bit(2, values);  /* GPIO 2 = high */
gpiod_set_array_value(gpios->ndescs, gpios->desc, gpios->info, values);

/* Get multiple GPIOs */
gpiod_get_array_value(gpios->ndescs, gpios->desc, gpios->info, values);
```

## Direction Control

```c
/* Change direction */
gpiod_direction_input(gpio);
gpiod_direction_output(gpio, initial_value);

/* Check current direction */
int dir = gpiod_get_direction(gpio);
/* 0 = output, 1 = input */
```

## GPIO to IRQ

```c
static int my_probe(struct platform_device *pdev)
{
    struct gpio_desc *irq_gpio;
    int irq;

    /* Get GPIO configured as input */
    irq_gpio = devm_gpiod_get(&pdev->dev, "irq", GPIOD_IN);
    if (IS_ERR(irq_gpio))
        return PTR_ERR(irq_gpio);

    /* Get IRQ number */
    irq = gpiod_to_irq(irq_gpio);
    if (irq < 0)
        return irq;

    /* Request interrupt */
    return devm_request_threaded_irq(&pdev->dev, irq,
                                     NULL, my_irq_handler,
                                     IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
                                     "my-device", dev);
}
```

## Reset Sequence Example

```c
static int device_reset(struct my_device *dev)
{
    if (!dev->reset_gpio)
        return 0;

    /* Assert reset */
    gpiod_set_value_cansleep(dev->reset_gpio, 1);

    /* Hold reset for 100us minimum */
    usleep_range(100, 200);

    /* Release reset */
    gpiod_set_value_cansleep(dev->reset_gpio, 0);

    /* Wait for device to initialize */
    usleep_range(1000, 2000);

    return 0;
}
```

## Power Sequence Example

```c
struct my_device {
    struct gpio_desc *power_gpio;
    struct gpio_desc *enable_gpio;
    struct gpio_desc *reset_gpio;
};

static int device_power_on(struct my_device *dev)
{
    /* Power on */
    if (dev->power_gpio)
        gpiod_set_value_cansleep(dev->power_gpio, 1);

    usleep_range(1000, 2000);  /* Power stabilization */

    /* Enable */
    if (dev->enable_gpio)
        gpiod_set_value_cansleep(dev->enable_gpio, 1);

    usleep_range(100, 200);

    /* Release reset */
    if (dev->reset_gpio) {
        gpiod_set_value_cansleep(dev->reset_gpio, 1);
        usleep_range(100, 200);
        gpiod_set_value_cansleep(dev->reset_gpio, 0);
    }

    usleep_range(5000, 10000);  /* Device startup */

    return 0;
}

static void device_power_off(struct my_device *dev)
{
    /* Assert reset */
    if (dev->reset_gpio)
        gpiod_set_value_cansleep(dev->reset_gpio, 1);

    /* Disable */
    if (dev->enable_gpio)
        gpiod_set_value_cansleep(dev->enable_gpio, 0);

    /* Power off */
    if (dev->power_gpio)
        gpiod_set_value_cansleep(dev->power_gpio, 0);
}
```

## GPIO Information

```c
/* Get GPIO label (from DT or request) */
const char *label = gpiod_get_label(gpio);

/* Check if GPIO is active low */
bool active_low = gpiod_is_active_low(gpio);

/* Get number of GPIOs in array */
int count = gpiod_count(dev, "data");
```

## Debouncing

```c
/* Set hardware debounce (if supported) */
int ret = gpiod_set_debounce(gpio, 1000);  /* 1000us */
if (ret == -ENOTSUPP)
    /* Use software debounce */
```

## Complete Example

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

struct button_led_device {
    struct device *dev;
    struct gpio_desc *led_gpio;
    struct gpio_desc *button_gpio;
    int button_irq;
    bool led_state;
};

static irqreturn_t button_irq_handler(int irq, void *data)
{
    struct button_led_device *dev = data;

    /* Toggle LED */
    dev->led_state = !dev->led_state;
    gpiod_set_value(dev->led_gpio, dev->led_state);

    dev_info(dev->dev, "Button pressed, LED %s\n",
             dev->led_state ? "on" : "off");

    return IRQ_HANDLED;
}

static int button_led_probe(struct platform_device *pdev)
{
    struct button_led_device *dev;
    int ret;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->dev = &pdev->dev;

    /* Get LED GPIO (output) */
    dev->led_gpio = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(dev->led_gpio))
        return dev_err_probe(&pdev->dev, PTR_ERR(dev->led_gpio),
                             "Failed to get LED GPIO\n");

    /* Get button GPIO (input) */
    dev->button_gpio = devm_gpiod_get(&pdev->dev, "button", GPIOD_IN);
    if (IS_ERR(dev->button_gpio))
        return dev_err_probe(&pdev->dev, PTR_ERR(dev->button_gpio),
                             "Failed to get button GPIO\n");

    /* Set up button debounce */
    gpiod_set_debounce(dev->button_gpio, 5000);  /* 5ms */

    /* Get IRQ for button */
    dev->button_irq = gpiod_to_irq(dev->button_gpio);
    if (dev->button_irq < 0)
        return dev->button_irq;

    /* Request IRQ */
    ret = devm_request_irq(&pdev->dev, dev->button_irq,
                           button_irq_handler,
                           IRQF_TRIGGER_FALLING,
                           "button", dev);
    if (ret)
        return ret;

    platform_set_drvdata(pdev, dev);

    dev_info(&pdev->dev, "Button-LED device initialized\n");
    return 0;
}

static const struct of_device_id button_led_of_match[] = {
    { .compatible = "demo,button-led" },
    { }
};
MODULE_DEVICE_TABLE(of, button_led_of_match);

static struct platform_driver button_led_driver = {
    .probe = button_led_probe,
    .driver = {
        .name = "button-led",
        .of_match_table = button_led_of_match,
    },
};
module_platform_driver(button_led_driver);

MODULE_LICENSE("GPL");
```

## Device Tree for Example

```dts
button_led {
    compatible = "demo,button-led";

    led-gpios = <&gpio0 5 GPIO_ACTIVE_HIGH>;
    button-gpios = <&gpio0 6 GPIO_ACTIVE_LOW>;
};
```

## Summary

- Use `devm_gpiod_get*()` for automatic cleanup
- `_optional` variants for non-critical GPIOs
- `gpiod_set_value()` handles active-low automatically
- Use `_cansleep` variants for I2C/SPI GPIO expanders
- `gpiod_to_irq()` converts GPIO to interrupt number
- Follow proper power/reset sequences with timing

## Next

Learn how to write a [GPIO provider]({% link part9/07-gpio-provider.md %}) (gpio_chip).
