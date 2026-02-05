---
layout: default
title: "10.2 PWM Provider"
parent: "Part 10: PWM, Watchdog, HWMON, LED Drivers"
nav_order: 2
---

# PWM Provider

PWM providers implement the `pwm_chip` structure to expose PWM channels to the kernel. This is needed when writing drivers for PWM controllers.

## Implementing pwm_ops

```c
#include <linux/pwm.h>

static int my_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
                        const struct pwm_state *state)
{
    struct my_pwm *priv = to_my_pwm(chip);
    u64 period_cycles, duty_cycles;
    u32 prescaler;

    /* Calculate hardware values */
    period_cycles = DIV_ROUND_UP_ULL(state->period * priv->clk_rate,
                                     NSEC_PER_SEC);
    duty_cycles = DIV_ROUND_UP_ULL(state->duty_cycle * priv->clk_rate,
                                   NSEC_PER_SEC);

    /* Find prescaler if period exceeds counter max */
    prescaler = DIV_ROUND_UP_ULL(period_cycles, priv->max_count);
    if (prescaler > priv->max_prescaler)
        return -EINVAL;

    period_cycles = DIV_ROUND_UP_ULL(period_cycles, prescaler);
    duty_cycles = DIV_ROUND_UP_ULL(duty_cycles, prescaler);

    /* Apply to hardware */
    writel(prescaler - 1, priv->regs + REG_PRESCALER(pwm->hwpwm));
    writel(period_cycles, priv->regs + REG_PERIOD(pwm->hwpwm));
    writel(duty_cycles, priv->regs + REG_DUTY(pwm->hwpwm));

    /* Handle polarity */
    if (state->polarity == PWM_POLARITY_INVERSED)
        my_pwm_set_polarity(priv, pwm->hwpwm, true);
    else
        my_pwm_set_polarity(priv, pwm->hwpwm, false);

    /* Enable/disable */
    if (state->enabled)
        my_pwm_enable_channel(priv, pwm->hwpwm);
    else
        my_pwm_disable_channel(priv, pwm->hwpwm);

    return 0;
}

static int my_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
                            struct pwm_state *state)
{
    struct my_pwm *priv = to_my_pwm(chip);
    u32 prescaler, period, duty;

    prescaler = readl(priv->regs + REG_PRESCALER(pwm->hwpwm)) + 1;
    period = readl(priv->regs + REG_PERIOD(pwm->hwpwm));
    duty = readl(priv->regs + REG_DUTY(pwm->hwpwm));

    state->period = DIV_ROUND_UP_ULL((u64)period * prescaler * NSEC_PER_SEC,
                                     priv->clk_rate);
    state->duty_cycle = DIV_ROUND_UP_ULL((u64)duty * prescaler * NSEC_PER_SEC,
                                         priv->clk_rate);
    state->polarity = my_pwm_get_polarity(priv, pwm->hwpwm) ?
                      PWM_POLARITY_INVERSED : PWM_POLARITY_NORMAL;
    state->enabled = my_pwm_is_enabled(priv, pwm->hwpwm);

    return 0;
}

static const struct pwm_ops my_pwm_ops = {
    .apply = my_pwm_apply,
    .get_state = my_pwm_get_state,
};
```

## Complete PWM Controller Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/clk.h>
#include <linux/io.h>

#define REG_CTRL        0x00
#define REG_PRESCALER   0x04
#define REG_PERIOD      0x08
#define REG_DUTY        0x0C
#define REG_ENABLE      0x10

#define CTRL_POLARITY   BIT(0)
#define NUM_CHANNELS    4
#define CHANNEL_OFFSET  0x20

struct my_pwm {
    struct pwm_chip chip;
    void __iomem *regs;
    struct clk *clk;
    unsigned long clk_rate;
    spinlock_t lock;
};

static inline struct my_pwm *to_my_pwm(struct pwm_chip *chip)
{
    return container_of(chip, struct my_pwm, chip);
}

static inline void __iomem *channel_base(struct my_pwm *priv, unsigned int ch)
{
    return priv->regs + ch * CHANNEL_OFFSET;
}

static void my_pwm_enable_channel(struct my_pwm *priv, unsigned int ch)
{
    void __iomem *base = channel_base(priv, ch);
    u32 val;

    val = readl(base + REG_ENABLE);
    val |= BIT(0);
    writel(val, base + REG_ENABLE);
}

static void my_pwm_disable_channel(struct my_pwm *priv, unsigned int ch)
{
    void __iomem *base = channel_base(priv, ch);
    u32 val;

    val = readl(base + REG_ENABLE);
    val &= ~BIT(0);
    writel(val, base + REG_ENABLE);
}

static bool my_pwm_is_enabled(struct my_pwm *priv, unsigned int ch)
{
    return readl(channel_base(priv, ch) + REG_ENABLE) & BIT(0);
}

static void my_pwm_set_polarity(struct my_pwm *priv, unsigned int ch, bool inv)
{
    void __iomem *base = channel_base(priv, ch);
    u32 val;

    val = readl(base + REG_CTRL);
    if (inv)
        val |= CTRL_POLARITY;
    else
        val &= ~CTRL_POLARITY;
    writel(val, base + REG_CTRL);
}

static bool my_pwm_get_polarity(struct my_pwm *priv, unsigned int ch)
{
    return readl(channel_base(priv, ch) + REG_CTRL) & CTRL_POLARITY;
}

static int my_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
                        const struct pwm_state *state)
{
    struct my_pwm *priv = to_my_pwm(chip);
    void __iomem *base = channel_base(priv, pwm->hwpwm);
    unsigned long flags;
    u64 period_cycles, duty_cycles;
    u32 prescaler;

    /* Convert nanoseconds to clock cycles */
    period_cycles = DIV_ROUND_CLOSEST_ULL(
        (u64)state->period * priv->clk_rate, NSEC_PER_SEC);
    duty_cycles = DIV_ROUND_CLOSEST_ULL(
        (u64)state->duty_cycle * priv->clk_rate, NSEC_PER_SEC);

    /* Calculate prescaler (assuming 16-bit counter) */
    prescaler = DIV_ROUND_UP_ULL(period_cycles, 0xFFFF);
    if (prescaler > 0xFFFF)
        return -ERANGE;

    if (prescaler > 0) {
        period_cycles = DIV_ROUND_CLOSEST_ULL(period_cycles, prescaler);
        duty_cycles = DIV_ROUND_CLOSEST_ULL(duty_cycles, prescaler);
    }

    spin_lock_irqsave(&priv->lock, flags);

    /* Disable while configuring */
    my_pwm_disable_channel(priv, pwm->hwpwm);

    /* Configure prescaler, period, duty */
    writel(prescaler ? prescaler - 1 : 0, base + REG_PRESCALER);
    writel(period_cycles, base + REG_PERIOD);
    writel(duty_cycles, base + REG_DUTY);

    /* Set polarity */
    my_pwm_set_polarity(priv, pwm->hwpwm,
                        state->polarity == PWM_POLARITY_INVERSED);

    /* Enable if requested */
    if (state->enabled)
        my_pwm_enable_channel(priv, pwm->hwpwm);

    spin_unlock_irqrestore(&priv->lock, flags);

    return 0;
}

static int my_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
                            struct pwm_state *state)
{
    struct my_pwm *priv = to_my_pwm(chip);
    void __iomem *base = channel_base(priv, pwm->hwpwm);
    u32 prescaler, period, duty;

    prescaler = readl(base + REG_PRESCALER) + 1;
    period = readl(base + REG_PERIOD);
    duty = readl(base + REG_DUTY);

    state->period = DIV_ROUND_CLOSEST_ULL(
        (u64)period * prescaler * NSEC_PER_SEC, priv->clk_rate);
    state->duty_cycle = DIV_ROUND_CLOSEST_ULL(
        (u64)duty * prescaler * NSEC_PER_SEC, priv->clk_rate);
    state->polarity = my_pwm_get_polarity(priv, pwm->hwpwm) ?
                      PWM_POLARITY_INVERSED : PWM_POLARITY_NORMAL;
    state->enabled = my_pwm_is_enabled(priv, pwm->hwpwm);

    return 0;
}

static int my_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
    struct my_pwm *priv = to_my_pwm(chip);

    /* Enable clock for this channel if needed */
    dev_dbg(chip->dev, "PWM channel %u requested\n", pwm->hwpwm);

    return 0;
}

static void my_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
    struct my_pwm *priv = to_my_pwm(chip);

    /* Disable the channel */
    my_pwm_disable_channel(priv, pwm->hwpwm);
    dev_dbg(chip->dev, "PWM channel %u freed\n", pwm->hwpwm);
}

static const struct pwm_ops my_pwm_ops = {
    .request = my_pwm_request,
    .free = my_pwm_free,
    .apply = my_pwm_apply,
    .get_state = my_pwm_get_state,
};

static int my_pwm_probe(struct platform_device *pdev)
{
    struct my_pwm *priv;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->regs = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(priv->regs))
        return PTR_ERR(priv->regs);

    priv->clk = devm_clk_get(&pdev->dev, NULL);
    if (IS_ERR(priv->clk))
        return dev_err_probe(&pdev->dev, PTR_ERR(priv->clk),
                             "Failed to get clock\n");

    ret = clk_prepare_enable(priv->clk);
    if (ret)
        return ret;

    priv->clk_rate = clk_get_rate(priv->clk);
    if (!priv->clk_rate) {
        clk_disable_unprepare(priv->clk);
        return -EINVAL;
    }

    spin_lock_init(&priv->lock);

    priv->chip.dev = &pdev->dev;
    priv->chip.ops = &my_pwm_ops;
    priv->chip.npwm = NUM_CHANNELS;

    ret = devm_pwmchip_add(&pdev->dev, &priv->chip);
    if (ret) {
        clk_disable_unprepare(priv->clk);
        return dev_err_probe(&pdev->dev, ret, "Failed to add PWM chip\n");
    }

    platform_set_drvdata(pdev, priv);

    dev_info(&pdev->dev, "PWM controller registered: %d channels @ %lu Hz\n",
             NUM_CHANNELS, priv->clk_rate);

    return 0;
}

static void my_pwm_remove(struct platform_device *pdev)
{
    struct my_pwm *priv = platform_get_drvdata(pdev);

    clk_disable_unprepare(priv->clk);
}

static int my_pwm_suspend(struct device *dev)
{
    struct my_pwm *priv = dev_get_drvdata(dev);

    clk_disable_unprepare(priv->clk);
    return 0;
}

static int my_pwm_resume(struct device *dev)
{
    struct my_pwm *priv = dev_get_drvdata(dev);

    return clk_prepare_enable(priv->clk);
}

static DEFINE_SIMPLE_DEV_PM_OPS(my_pwm_pm_ops, my_pwm_suspend, my_pwm_resume);

static const struct of_device_id my_pwm_of_match[] = {
    { .compatible = "vendor,my-pwm" },
    { }
};
MODULE_DEVICE_TABLE(of, my_pwm_of_match);

static struct platform_driver my_pwm_driver = {
    .probe = my_pwm_probe,
    .remove = my_pwm_remove,
    .driver = {
        .name = "my-pwm",
        .of_match_table = my_pwm_of_match,
        .pm = pm_sleep_ptr(&my_pwm_pm_ops),
    },
};
module_platform_driver(my_pwm_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Example PWM Controller Driver");
```

## Device Tree Binding

```dts
pwm0: pwm@10000000 {
    compatible = "vendor,my-pwm";
    reg = <0x10000000 0x100>;
    #pwm-cells = <2>;           /* channel, period */
    clocks = <&clk_pwm>;
    clock-names = "pwm";
};

/* Alternative with 3 cells for polarity */
pwm1: pwm@10001000 {
    compatible = "vendor,my-pwm";
    reg = <0x10001000 0x100>;
    #pwm-cells = <3>;           /* channel, period, flags */
    clocks = <&clk_pwm>;
};
```

## Handling PWM Cells

For custom cell formats, implement `of_xlate`:

```c
static int my_pwm_xlate(struct pwm_chip *chip,
                        const struct of_phandle_args *args)
{
    if (args->args_count < 2)
        return -EINVAL;

    if (args->args[0] >= chip->npwm)
        return -EINVAL;

    return args->args[0];  /* Return channel number */
}

/* In probe */
priv->chip.of_xlate = my_pwm_xlate;
priv->chip.of_pwm_n_cells = 2;
```

## PWM Capture (Input Mode)

Some PWM controllers support capture mode:

```c
static int my_pwm_capture(struct pwm_chip *chip, struct pwm_device *pwm,
                          struct pwm_capture *result, unsigned long timeout)
{
    struct my_pwm *priv = to_my_pwm(chip);
    u32 period, duty;

    /* Configure capture mode */
    writel(CAPTURE_ENABLE, channel_base(priv, pwm->hwpwm) + REG_CTRL);

    /* Wait for capture complete (simplified) */
    /* In real driver, use interrupt or poll with timeout */
    msleep(timeout);

    period = readl(channel_base(priv, pwm->hwpwm) + REG_CAPTURE_PERIOD);
    duty = readl(channel_base(priv, pwm->hwpwm) + REG_CAPTURE_DUTY);

    result->period = DIV_ROUND_CLOSEST_ULL(
        (u64)period * NSEC_PER_SEC, priv->clk_rate);
    result->duty_cycle = DIV_ROUND_CLOSEST_ULL(
        (u64)duty * NSEC_PER_SEC, priv->clk_rate);

    return 0;
}
```

## Registration Methods

### Modern (Managed)

```c
/* Preferred: automatic cleanup */
ret = devm_pwmchip_add(&pdev->dev, &priv->chip);
```

### Legacy (Manual)

```c
/* Must call pwmchip_remove() in remove/error path */
ret = pwmchip_add(&priv->chip);
if (ret)
    return ret;

/* In remove */
pwmchip_remove(&priv->chip);
```

## Summary

- Implement `apply()` to configure period, duty cycle, polarity, enable
- Implement `get_state()` to read current configuration
- Use `devm_pwmchip_add()` for automatic cleanup
- Handle clock management for frequency calculations
- Use spinlock for register access protection
- Support both 2-cell and 3-cell DT formats

## Further Reading

- [PWM API Documentation](https://docs.kernel.org/driver-api/pwm.html) - Official kernel docs
- [PWM Drivers](https://elixir.bootlin.com/linux/v6.6/source/drivers/pwm) - Example implementations
- [pwm-simple.c](https://elixir.bootlin.com/linux/v6.6/source/drivers/pwm/pwm-stm32.c) - STM32 PWM driver

## Next

Learn about the [Watchdog subsystem]({% link part10/03-watchdog-subsystem.md %}).
