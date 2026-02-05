---
layout: default
title: "9.7 GPIO Provider"
parent: "Part 9: I2C, SPI, and GPIO Drivers"
nav_order: 7
---

# GPIO Provider

GPIO providers implement the `gpio_chip` structure to expose GPIO pins to the kernel. This is needed when writing drivers for GPIO controllers or expanders.

## gpio_chip Structure

```c
#include <linux/gpio/driver.h>

struct gpio_chip {
    const char *label;
    struct device *parent;
    struct module *owner;
    int base;                      /* First GPIO number (-1 for auto) */
    u16 ngpio;                     /* Number of GPIOs */

    /* Callbacks */
    int (*request)(struct gpio_chip *gc, unsigned int offset);
    void (*free)(struct gpio_chip *gc, unsigned int offset);
    int (*get_direction)(struct gpio_chip *gc, unsigned int offset);
    int (*direction_input)(struct gpio_chip *gc, unsigned int offset);
    int (*direction_output)(struct gpio_chip *gc, unsigned int offset, int value);
    int (*get)(struct gpio_chip *gc, unsigned int offset);
    void (*set)(struct gpio_chip *gc, unsigned int offset, int value);
    void (*set_multiple)(struct gpio_chip *gc, unsigned long *mask,
                         unsigned long *bits);
    int (*set_config)(struct gpio_chip *gc, unsigned int offset,
                      unsigned long config);
    int (*to_irq)(struct gpio_chip *gc, unsigned int offset);

    /* For Device Tree */
    struct fwnode_handle *fwnode;
    bool can_sleep;               /* Set for I2C/SPI expanders */
};
```

## Simple GPIO Controller

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>

#define GPIO_DATA       0x00    /* Data register */
#define GPIO_DIR        0x04    /* Direction register (1=output) */

struct my_gpio {
    struct gpio_chip gc;
    void __iomem *regs;
    spinlock_t lock;
};

static int my_gpio_get_direction(struct gpio_chip *gc, unsigned int offset)
{
    struct my_gpio *gpio = gpiochip_get_data(gc);
    u32 dir;

    dir = readl(gpio->regs + GPIO_DIR);
    /* Return GPIOF_DIR_IN (1) or GPIOF_DIR_OUT (0) */
    return !(dir & BIT(offset));
}

static int my_gpio_direction_input(struct gpio_chip *gc, unsigned int offset)
{
    struct my_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    u32 dir;

    spin_lock_irqsave(&gpio->lock, flags);

    dir = readl(gpio->regs + GPIO_DIR);
    dir &= ~BIT(offset);  /* Clear bit = input */
    writel(dir, gpio->regs + GPIO_DIR);

    spin_unlock_irqrestore(&gpio->lock, flags);

    return 0;
}

static int my_gpio_direction_output(struct gpio_chip *gc,
                                    unsigned int offset, int value)
{
    struct my_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    u32 dir, data;

    spin_lock_irqsave(&gpio->lock, flags);

    /* Set value first */
    data = readl(gpio->regs + GPIO_DATA);
    if (value)
        data |= BIT(offset);
    else
        data &= ~BIT(offset);
    writel(data, gpio->regs + GPIO_DATA);

    /* Then set direction */
    dir = readl(gpio->regs + GPIO_DIR);
    dir |= BIT(offset);  /* Set bit = output */
    writel(dir, gpio->regs + GPIO_DIR);

    spin_unlock_irqrestore(&gpio->lock, flags);

    return 0;
}

static int my_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
    struct my_gpio *gpio = gpiochip_get_data(gc);

    return !!(readl(gpio->regs + GPIO_DATA) & BIT(offset));
}

static void my_gpio_set(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct my_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    u32 data;

    spin_lock_irqsave(&gpio->lock, flags);

    data = readl(gpio->regs + GPIO_DATA);
    if (value)
        data |= BIT(offset);
    else
        data &= ~BIT(offset);
    writel(data, gpio->regs + GPIO_DATA);

    spin_unlock_irqrestore(&gpio->lock, flags);
}

static void my_gpio_set_multiple(struct gpio_chip *gc,
                                 unsigned long *mask,
                                 unsigned long *bits)
{
    struct my_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    u32 data;

    spin_lock_irqsave(&gpio->lock, flags);

    data = readl(gpio->regs + GPIO_DATA);
    data = (data & ~(*mask)) | (*bits & *mask);
    writel(data, gpio->regs + GPIO_DATA);

    spin_unlock_irqrestore(&gpio->lock, flags);
}

static int my_gpio_probe(struct platform_device *pdev)
{
    struct my_gpio *gpio;
    int ret;

    gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
    if (!gpio)
        return -ENOMEM;

    gpio->regs = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(gpio->regs))
        return PTR_ERR(gpio->regs);

    spin_lock_init(&gpio->lock);

    /* Configure gpio_chip */
    gpio->gc.label = dev_name(&pdev->dev);
    gpio->gc.parent = &pdev->dev;
    gpio->gc.owner = THIS_MODULE;
    gpio->gc.base = -1;  /* Auto-assign */
    gpio->gc.ngpio = 32;
    gpio->gc.get_direction = my_gpio_get_direction;
    gpio->gc.direction_input = my_gpio_direction_input;
    gpio->gc.direction_output = my_gpio_direction_output;
    gpio->gc.get = my_gpio_get;
    gpio->gc.set = my_gpio_set;
    gpio->gc.set_multiple = my_gpio_set_multiple;

    ret = devm_gpiochip_add_data(&pdev->dev, &gpio->gc, gpio);
    if (ret)
        return dev_err_probe(&pdev->dev, ret, "Failed to add GPIO chip\n");

    dev_info(&pdev->dev, "GPIO controller registered with %d pins\n",
             gpio->gc.ngpio);

    return 0;
}

static const struct of_device_id my_gpio_of_match[] = {
    { .compatible = "vendor,my-gpio" },
    { }
};
MODULE_DEVICE_TABLE(of, my_gpio_of_match);

static struct platform_driver my_gpio_driver = {
    .probe = my_gpio_probe,
    .driver = {
        .name = "my-gpio",
        .of_match_table = my_gpio_of_match,
    },
};
module_platform_driver(my_gpio_driver);

MODULE_LICENSE("GPL");
```

## I2C GPIO Expander

For I2C/SPI expanders, set `can_sleep = true`:

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio/driver.h>
#include <linux/regmap.h>

#define REG_INPUT   0x00
#define REG_OUTPUT  0x01
#define REG_CONFIG  0x03  /* 0=output, 1=input */

struct i2c_gpio_expander {
    struct gpio_chip gc;
    struct regmap *regmap;
    struct mutex lock;
};

static int expander_get_direction(struct gpio_chip *gc, unsigned int offset)
{
    struct i2c_gpio_expander *exp = gpiochip_get_data(gc);
    unsigned int val;
    int ret;

    ret = regmap_read(exp->regmap, REG_CONFIG, &val);
    if (ret)
        return ret;

    return !!(val & BIT(offset));  /* 1 = input */
}

static int expander_direction_input(struct gpio_chip *gc, unsigned int offset)
{
    struct i2c_gpio_expander *exp = gpiochip_get_data(gc);

    return regmap_set_bits(exp->regmap, REG_CONFIG, BIT(offset));
}

static int expander_direction_output(struct gpio_chip *gc,
                                     unsigned int offset, int value)
{
    struct i2c_gpio_expander *exp = gpiochip_get_data(gc);
    int ret;

    mutex_lock(&exp->lock);

    /* Set output value */
    ret = regmap_update_bits(exp->regmap, REG_OUTPUT, BIT(offset),
                             value ? BIT(offset) : 0);
    if (ret)
        goto out;

    /* Set as output */
    ret = regmap_clear_bits(exp->regmap, REG_CONFIG, BIT(offset));

out:
    mutex_unlock(&exp->lock);
    return ret;
}

static int expander_get(struct gpio_chip *gc, unsigned int offset)
{
    struct i2c_gpio_expander *exp = gpiochip_get_data(gc);
    unsigned int val;
    int ret;

    ret = regmap_read(exp->regmap, REG_INPUT, &val);
    if (ret)
        return ret;

    return !!(val & BIT(offset));
}

static void expander_set(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct i2c_gpio_expander *exp = gpiochip_get_data(gc);

    regmap_update_bits(exp->regmap, REG_OUTPUT, BIT(offset),
                       value ? BIT(offset) : 0);
}

static const struct regmap_config expander_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x07,
};

static int expander_probe(struct i2c_client *client)
{
    struct i2c_gpio_expander *exp;
    int ret;

    exp = devm_kzalloc(&client->dev, sizeof(*exp), GFP_KERNEL);
    if (!exp)
        return -ENOMEM;

    exp->regmap = devm_regmap_init_i2c(client, &expander_regmap_config);
    if (IS_ERR(exp->regmap))
        return PTR_ERR(exp->regmap);

    mutex_init(&exp->lock);

    exp->gc.label = client->name;
    exp->gc.parent = &client->dev;
    exp->gc.owner = THIS_MODULE;
    exp->gc.base = -1;
    exp->gc.ngpio = 8;
    exp->gc.can_sleep = true;  /* Important for I2C! */
    exp->gc.get_direction = expander_get_direction;
    exp->gc.direction_input = expander_direction_input;
    exp->gc.direction_output = expander_direction_output;
    exp->gc.get = expander_get;
    exp->gc.set = expander_set;

    ret = devm_gpiochip_add_data(&client->dev, &exp->gc, exp);
    if (ret)
        return ret;

    i2c_set_clientdata(client, exp);

    dev_info(&client->dev, "I2C GPIO expander registered\n");
    return 0;
}

static const struct of_device_id expander_of_match[] = {
    { .compatible = "vendor,gpio-expander" },
    { }
};
MODULE_DEVICE_TABLE(of, expander_of_match);

static struct i2c_driver expander_driver = {
    .driver = {
        .name = "gpio-expander",
        .of_match_table = expander_of_match,
    },
    .probe = expander_probe,
};
module_i2c_driver(expander_driver);

MODULE_LICENSE("GPL");
```

## Device Tree Binding

```dts
/* Memory-mapped GPIO controller */
gpio0: gpio@10000000 {
    compatible = "vendor,my-gpio";
    reg = <0x10000000 0x100>;
    gpio-controller;
    #gpio-cells = <2>;
};

/* I2C GPIO expander */
&i2c1 {
    gpio_exp: gpio@20 {
        compatible = "vendor,gpio-expander";
        reg = <0x20>;
        gpio-controller;
        #gpio-cells = <2>;
    };
};

/* Using the GPIOs */
led_device {
    compatible = "gpio-leds";

    led0 {
        gpios = <&gpio0 5 GPIO_ACTIVE_HIGH>;
        label = "led0";
    };

    led1 {
        gpios = <&gpio_exp 0 GPIO_ACTIVE_LOW>;
        label = "led1";
    };
};
```

## Adding Interrupt Support

```c
static int my_gpio_to_irq(struct gpio_chip *gc, unsigned int offset)
{
    struct my_gpio *gpio = gpiochip_get_data(gc);

    return irq_find_mapping(gpio->irq_domain, offset);
}

static int my_gpio_probe(struct platform_device *pdev)
{
    struct gpio_irq_chip *girq;

    /* ... gpio_chip setup ... */

    /* Setup interrupt support */
    girq = &gpio->gc.irq;
    gpio_irq_chip_set_chip(girq, &my_gpio_irqchip);
    girq->parent_handler = my_gpio_irq_handler;
    girq->num_parents = 1;
    girq->parents = devm_kcalloc(&pdev->dev, 1,
                                  sizeof(*girq->parents), GFP_KERNEL);
    girq->parents[0] = platform_get_irq(pdev, 0);
    girq->default_type = IRQ_TYPE_NONE;
    girq->handler = handle_edge_irq;

    return devm_gpiochip_add_data(&pdev->dev, &gpio->gc, gpio);
}
```

## GPIO Configuration

```c
static int my_gpio_set_config(struct gpio_chip *gc, unsigned int offset,
                              unsigned long config)
{
    struct my_gpio *gpio = gpiochip_get_data(gc);
    enum pin_config_param param = pinconf_to_config_param(config);

    switch (param) {
    case PIN_CONFIG_BIAS_PULL_UP:
        return set_pull_up(gpio, offset);
    case PIN_CONFIG_BIAS_PULL_DOWN:
        return set_pull_down(gpio, offset);
    case PIN_CONFIG_BIAS_DISABLE:
        return disable_pull(gpio, offset);
    case PIN_CONFIG_INPUT_DEBOUNCE:
        return set_debounce(gpio, offset,
                            pinconf_to_config_argument(config));
    default:
        return -ENOTSUPP;
    }
}
```

## Summary

- Implement `gpio_chip` structure for GPIO controllers
- Set `can_sleep = true` for I2C/SPI expanders
- Use `devm_gpiochip_add_data()` for registration
- Implement at minimum: direction_input/output, get, set
- Add `#gpio-cells = <2>` to Device Tree binding
- Use locking (spinlock or mutex) for register access
- For IRQ support, configure `gpio_irq_chip`

## Next

Continue to [Part 10: PWM, Watchdog, HWMON, LED Drivers]({% link part10/index.md %}) to learn about more device subsystems.
