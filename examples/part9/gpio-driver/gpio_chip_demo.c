// SPDX-License-Identifier: GPL-2.0
/*
 * GPIO Chip Demo Driver
 *
 * Demonstrates implementing a gpio_chip provider as a platform driver.
 * Creates a virtual GPIO controller with 8 pins for testing purposes.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/debugfs.h>

#define DRIVER_NAME     "demo-gpio"
#define NUM_GPIOS       8

/* Virtual register offsets */
#define REG_DATA        0x00    /* Data register */
#define REG_DIR         0x04    /* Direction (1=output, 0=input) */
#define REG_IRQ_EN      0x08    /* IRQ enable */
#define REG_IRQ_TYPE    0x0C    /* IRQ type (edge/level) */
#define REG_IRQ_POL     0x10    /* IRQ polarity */
#define REG_IRQ_STATUS  0x14    /* IRQ status (pending) */

struct demo_gpio {
    struct gpio_chip gc;
    spinlock_t lock;

    /* Virtual registers */
    u32 reg_data;
    u32 reg_dir;
    u32 reg_irq_en;
    u32 reg_irq_type;
    u32 reg_irq_pol;
    u32 reg_irq_status;

    /* IRQ support */
    struct irq_chip irq_chip;
    int irq;

    /* Debugfs */
    struct dentry *debugfs;
};

/* GPIO operations */
static int demo_gpio_get_direction(struct gpio_chip *gc, unsigned int offset)
{
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    int dir;

    spin_lock_irqsave(&gpio->lock, flags);
    /* Return GPIO_LINE_DIRECTION_IN (1) or GPIO_LINE_DIRECTION_OUT (0) */
    dir = !(gpio->reg_dir & BIT(offset));
    spin_unlock_irqrestore(&gpio->lock, flags);

    return dir;
}

static int demo_gpio_direction_input(struct gpio_chip *gc, unsigned int offset)
{
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;

    spin_lock_irqsave(&gpio->lock, flags);
    gpio->reg_dir &= ~BIT(offset);
    spin_unlock_irqrestore(&gpio->lock, flags);

    dev_dbg(gc->parent, "GPIO %u set to input\n", offset);
    return 0;
}

static int demo_gpio_direction_output(struct gpio_chip *gc,
                                      unsigned int offset, int value)
{
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;

    spin_lock_irqsave(&gpio->lock, flags);

    /* Set value first */
    if (value)
        gpio->reg_data |= BIT(offset);
    else
        gpio->reg_data &= ~BIT(offset);

    /* Then set direction */
    gpio->reg_dir |= BIT(offset);

    spin_unlock_irqrestore(&gpio->lock, flags);

    dev_dbg(gc->parent, "GPIO %u set to output, value=%d\n", offset, value);
    return 0;
}

static int demo_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    int value;

    spin_lock_irqsave(&gpio->lock, flags);
    value = !!(gpio->reg_data & BIT(offset));
    spin_unlock_irqrestore(&gpio->lock, flags);

    return value;
}

static void demo_gpio_set(struct gpio_chip *gc, unsigned int offset, int value)
{
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;

    spin_lock_irqsave(&gpio->lock, flags);
    if (value)
        gpio->reg_data |= BIT(offset);
    else
        gpio->reg_data &= ~BIT(offset);
    spin_unlock_irqrestore(&gpio->lock, flags);

    dev_dbg(gc->parent, "GPIO %u set to %d\n", offset, value);
}

static void demo_gpio_set_multiple(struct gpio_chip *gc,
                                   unsigned long *mask,
                                   unsigned long *bits)
{
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;

    spin_lock_irqsave(&gpio->lock, flags);
    gpio->reg_data = (gpio->reg_data & ~(*mask)) | (*bits & *mask);
    spin_unlock_irqrestore(&gpio->lock, flags);

    dev_dbg(gc->parent, "GPIO multiple set: mask=0x%lx, bits=0x%lx\n",
            *mask, *bits);
}

static int demo_gpio_get_multiple(struct gpio_chip *gc,
                                  unsigned long *mask,
                                  unsigned long *bits)
{
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;

    spin_lock_irqsave(&gpio->lock, flags);
    *bits = gpio->reg_data & *mask;
    spin_unlock_irqrestore(&gpio->lock, flags);

    return 0;
}

/* IRQ chip operations */
static void demo_irq_mask(struct irq_data *d)
{
    struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    irq_hw_number_t hwirq = irqd_to_hwirq(d);

    spin_lock_irqsave(&gpio->lock, flags);
    gpio->reg_irq_en &= ~BIT(hwirq);
    spin_unlock_irqrestore(&gpio->lock, flags);

    gpiochip_disable_irq(gc, hwirq);
}

static void demo_irq_unmask(struct irq_data *d)
{
    struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    irq_hw_number_t hwirq = irqd_to_hwirq(d);

    gpiochip_enable_irq(gc, hwirq);

    spin_lock_irqsave(&gpio->lock, flags);
    gpio->reg_irq_en |= BIT(hwirq);
    spin_unlock_irqrestore(&gpio->lock, flags);
}

static int demo_irq_set_type(struct irq_data *d, unsigned int type)
{
    struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    irq_hw_number_t hwirq = irqd_to_hwirq(d);

    spin_lock_irqsave(&gpio->lock, flags);

    switch (type) {
    case IRQ_TYPE_EDGE_RISING:
        gpio->reg_irq_type |= BIT(hwirq);   /* Edge */
        gpio->reg_irq_pol |= BIT(hwirq);    /* Rising */
        break;
    case IRQ_TYPE_EDGE_FALLING:
        gpio->reg_irq_type |= BIT(hwirq);   /* Edge */
        gpio->reg_irq_pol &= ~BIT(hwirq);   /* Falling */
        break;
    case IRQ_TYPE_LEVEL_HIGH:
        gpio->reg_irq_type &= ~BIT(hwirq);  /* Level */
        gpio->reg_irq_pol |= BIT(hwirq);    /* High */
        break;
    case IRQ_TYPE_LEVEL_LOW:
        gpio->reg_irq_type &= ~BIT(hwirq);  /* Level */
        gpio->reg_irq_pol &= ~BIT(hwirq);   /* Low */
        break;
    default:
        spin_unlock_irqrestore(&gpio->lock, flags);
        return -EINVAL;
    }

    spin_unlock_irqrestore(&gpio->lock, flags);
    return 0;
}

static void demo_irq_ack(struct irq_data *d)
{
    struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
    struct demo_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    irq_hw_number_t hwirq = irqd_to_hwirq(d);

    spin_lock_irqsave(&gpio->lock, flags);
    gpio->reg_irq_status &= ~BIT(hwirq);
    spin_unlock_irqrestore(&gpio->lock, flags);
}

static const struct irq_chip demo_irq_chip_template = {
    .name = "demo-gpio-irq",
    .irq_mask = demo_irq_mask,
    .irq_unmask = demo_irq_unmask,
    .irq_set_type = demo_irq_set_type,
    .irq_ack = demo_irq_ack,
    .flags = IRQCHIP_IMMUTABLE,
    GPIOCHIP_IRQ_RESOURCE_HELPERS,
};

/* Debugfs interface for testing */
static int demo_gpio_debugfs_show(struct seq_file *s, void *unused)
{
    struct demo_gpio *gpio = s->private;
    int i;

    seq_printf(s, "Demo GPIO Controller Status\n");
    seq_printf(s, "===========================\n\n");

    seq_printf(s, "Virtual Registers:\n");
    seq_printf(s, "  DATA:       0x%02x\n", gpio->reg_data & 0xFF);
    seq_printf(s, "  DIR:        0x%02x (1=out, 0=in)\n", gpio->reg_dir & 0xFF);
    seq_printf(s, "  IRQ_EN:     0x%02x\n", gpio->reg_irq_en & 0xFF);
    seq_printf(s, "  IRQ_TYPE:   0x%02x (1=edge, 0=level)\n", gpio->reg_irq_type & 0xFF);
    seq_printf(s, "  IRQ_POL:    0x%02x (1=rising/high, 0=falling/low)\n", gpio->reg_irq_pol & 0xFF);
    seq_printf(s, "  IRQ_STATUS: 0x%02x\n", gpio->reg_irq_status & 0xFF);

    seq_printf(s, "\nGPIO Pin Status:\n");
    seq_printf(s, "  PIN  DIR    VALUE\n");
    for (i = 0; i < NUM_GPIOS; i++) {
        seq_printf(s, "  %d    %s  %d\n",
                   i,
                   (gpio->reg_dir & BIT(i)) ? "out" : "in ",
                   !!(gpio->reg_data & BIT(i)));
    }

    return 0;
}

static int demo_gpio_debugfs_open(struct inode *inode, struct file *file)
{
    return single_open(file, demo_gpio_debugfs_show, inode->i_private);
}

/* Allow writing to simulate input changes */
static ssize_t demo_gpio_debugfs_write(struct file *file,
                                       const char __user *user_buf,
                                       size_t count, loff_t *ppos)
{
    struct seq_file *s = file->private_data;
    struct demo_gpio *gpio = s->private;
    char buf[32];
    unsigned int pin, value;
    unsigned long flags;

    if (count >= sizeof(buf))
        return -EINVAL;

    if (copy_from_user(buf, user_buf, count))
        return -EFAULT;

    buf[count] = '\0';

    /* Format: "pin value" - simulate input change */
    if (sscanf(buf, "%u %u", &pin, &value) != 2)
        return -EINVAL;

    if (pin >= NUM_GPIOS)
        return -EINVAL;

    spin_lock_irqsave(&gpio->lock, flags);

    /* Only allow changing inputs */
    if (!(gpio->reg_dir & BIT(pin))) {
        if (value)
            gpio->reg_data |= BIT(pin);
        else
            gpio->reg_data &= ~BIT(pin);

        dev_info(gpio->gc.parent, "Simulated input change: GPIO %u = %u\n",
                 pin, value);
    } else {
        spin_unlock_irqrestore(&gpio->lock, flags);
        return -EPERM;  /* Can't change output via debugfs */
    }

    spin_unlock_irqrestore(&gpio->lock, flags);

    return count;
}

static const struct file_operations demo_gpio_debugfs_fops = {
    .open = demo_gpio_debugfs_open,
    .read = seq_read,
    .write = demo_gpio_debugfs_write,
    .llseek = seq_lseek,
    .release = single_release,
};

static int demo_gpio_probe(struct platform_device *pdev)
{
    struct demo_gpio *gpio;
    struct gpio_irq_chip *girq;
    int ret;

    gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
    if (!gpio)
        return -ENOMEM;

    spin_lock_init(&gpio->lock);

    /* Initialize virtual registers */
    gpio->reg_data = 0;
    gpio->reg_dir = 0;  /* All inputs by default */
    gpio->reg_irq_en = 0;
    gpio->reg_irq_type = 0;
    gpio->reg_irq_pol = 0;
    gpio->reg_irq_status = 0;

    /* Configure gpio_chip */
    gpio->gc.label = DRIVER_NAME;
    gpio->gc.parent = &pdev->dev;
    gpio->gc.owner = THIS_MODULE;
    gpio->gc.base = -1;  /* Dynamic base allocation */
    gpio->gc.ngpio = NUM_GPIOS;
    gpio->gc.get_direction = demo_gpio_get_direction;
    gpio->gc.direction_input = demo_gpio_direction_input;
    gpio->gc.direction_output = demo_gpio_direction_output;
    gpio->gc.get = demo_gpio_get;
    gpio->gc.set = demo_gpio_set;
    gpio->gc.set_multiple = demo_gpio_set_multiple;
    gpio->gc.get_multiple = demo_gpio_get_multiple;
    gpio->gc.can_sleep = false;

    /* Setup IRQ chip */
    girq = &gpio->gc.irq;
    gpio_irq_chip_set_chip(girq, &demo_irq_chip_template);
    girq->handler = handle_simple_irq;
    girq->default_type = IRQ_TYPE_NONE;

    /* Register GPIO chip */
    ret = devm_gpiochip_add_data(&pdev->dev, &gpio->gc, gpio);
    if (ret)
        return dev_err_probe(&pdev->dev, ret, "Failed to add GPIO chip\n");

    platform_set_drvdata(pdev, gpio);

    /* Create debugfs entry */
    gpio->debugfs = debugfs_create_file("demo_gpio", 0644, NULL,
                                        gpio, &demo_gpio_debugfs_fops);

    dev_info(&pdev->dev, "Demo GPIO controller registered: %d GPIOs, base=%d\n",
             gpio->gc.ngpio, gpio->gc.base);

    return 0;
}

static void demo_gpio_remove(struct platform_device *pdev)
{
    struct demo_gpio *gpio = platform_get_drvdata(pdev);

    debugfs_remove(gpio->debugfs);
    dev_info(&pdev->dev, "Demo GPIO controller removed\n");
}

static const struct of_device_id demo_gpio_of_match[] = {
    { .compatible = "demo,gpio-controller" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_gpio_of_match);

static struct platform_driver demo_gpio_driver = {
    .probe = demo_gpio_probe,
    .remove = demo_gpio_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = demo_gpio_of_match,
    },
};

/* Platform device for testing without Device Tree */
static struct platform_device *demo_pdev;

static int __init demo_gpio_init(void)
{
    int ret;

    ret = platform_driver_register(&demo_gpio_driver);
    if (ret)
        return ret;

    /* Create a platform device for testing */
    demo_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(demo_pdev)) {
        platform_driver_unregister(&demo_gpio_driver);
        return PTR_ERR(demo_pdev);
    }

    return 0;
}

static void __exit demo_gpio_exit(void)
{
    platform_device_unregister(demo_pdev);
    platform_driver_unregister(&demo_gpio_driver);
}

module_init(demo_gpio_init);
module_exit(demo_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Demo GPIO Chip Driver");
