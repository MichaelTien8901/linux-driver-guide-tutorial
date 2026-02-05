// SPDX-License-Identifier: GPL-2.0
/*
 * PWM Controller Demo Driver
 *
 * Demonstrates implementing a pwm_chip for a virtual PWM controller.
 * Creates 4 virtual PWM channels accessible via /sys/class/pwm/.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/debugfs.h>

#define DRIVER_NAME     "demo-pwm"
#define NUM_CHANNELS    4

struct demo_pwm_channel {
    u64 period;
    u64 duty_cycle;
    enum pwm_polarity polarity;
    bool enabled;
};

struct demo_pwm {
    struct pwm_chip chip;
    struct demo_pwm_channel channels[NUM_CHANNELS];
    struct dentry *debugfs;
    spinlock_t lock;
};

static inline struct demo_pwm *to_demo_pwm(struct pwm_chip *chip)
{
    return container_of(chip, struct demo_pwm, chip);
}

static int demo_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
    dev_dbg(chip->dev, "PWM channel %u requested\n", pwm->hwpwm);
    return 0;
}

static void demo_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
    struct demo_pwm *priv = to_demo_pwm(chip);
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    priv->channels[pwm->hwpwm].enabled = false;
    priv->channels[pwm->hwpwm].duty_cycle = 0;
    spin_unlock_irqrestore(&priv->lock, flags);

    dev_dbg(chip->dev, "PWM channel %u freed\n", pwm->hwpwm);
}

static int demo_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
                          const struct pwm_state *state)
{
    struct demo_pwm *priv = to_demo_pwm(chip);
    struct demo_pwm_channel *ch = &priv->channels[pwm->hwpwm];
    unsigned long flags;

    /* Validate parameters */
    if (state->period == 0)
        return -EINVAL;

    if (state->duty_cycle > state->period)
        return -EINVAL;

    spin_lock_irqsave(&priv->lock, flags);

    ch->period = state->period;
    ch->duty_cycle = state->duty_cycle;
    ch->polarity = state->polarity;
    ch->enabled = state->enabled;

    spin_unlock_irqrestore(&priv->lock, flags);

    dev_dbg(chip->dev, "PWM%u: period=%llu duty=%llu pol=%d en=%d\n",
            pwm->hwpwm, state->period, state->duty_cycle,
            state->polarity, state->enabled);

    return 0;
}

static int demo_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
                              struct pwm_state *state)
{
    struct demo_pwm *priv = to_demo_pwm(chip);
    struct demo_pwm_channel *ch = &priv->channels[pwm->hwpwm];
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);

    state->period = ch->period;
    state->duty_cycle = ch->duty_cycle;
    state->polarity = ch->polarity;
    state->enabled = ch->enabled;

    spin_unlock_irqrestore(&priv->lock, flags);

    return 0;
}

static const struct pwm_ops demo_pwm_ops = {
    .request = demo_pwm_request,
    .free = demo_pwm_free,
    .apply = demo_pwm_apply,
    .get_state = demo_pwm_get_state,
};

/* Debugfs interface */
static int demo_pwm_debugfs_show(struct seq_file *s, void *unused)
{
    struct demo_pwm *priv = s->private;
    int i;

    seq_printf(s, "Demo PWM Controller Status\n");
    seq_printf(s, "==========================\n\n");
    seq_printf(s, "CH  PERIOD(ns)     DUTY(ns)       DUTY%%  POL  EN\n");
    seq_printf(s, "--- -------------- -------------- ------ ---- ---\n");

    for (i = 0; i < NUM_CHANNELS; i++) {
        struct demo_pwm_channel *ch = &priv->channels[i];
        u64 duty_percent = 0;

        if (ch->period > 0)
            duty_percent = div64_u64(ch->duty_cycle * 100, ch->period);

        seq_printf(s, "%2d  %14llu %14llu %5llu%% %4s %3s\n",
                   i, ch->period, ch->duty_cycle, duty_percent,
                   ch->polarity == PWM_POLARITY_INVERSED ? "inv" : "nor",
                   ch->enabled ? "yes" : "no");
    }

    return 0;
}

static int demo_pwm_debugfs_open(struct inode *inode, struct file *file)
{
    return single_open(file, demo_pwm_debugfs_show, inode->i_private);
}

static const struct file_operations demo_pwm_debugfs_fops = {
    .open = demo_pwm_debugfs_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int demo_pwm_probe(struct platform_device *pdev)
{
    struct demo_pwm *priv;
    int ret, i;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    spin_lock_init(&priv->lock);

    /* Initialize channels with default values */
    for (i = 0; i < NUM_CHANNELS; i++) {
        priv->channels[i].period = 1000000;  /* 1ms default */
        priv->channels[i].duty_cycle = 0;
        priv->channels[i].polarity = PWM_POLARITY_NORMAL;
        priv->channels[i].enabled = false;
    }

    /* Setup PWM chip */
    priv->chip.dev = &pdev->dev;
    priv->chip.ops = &demo_pwm_ops;
    priv->chip.npwm = NUM_CHANNELS;

    ret = devm_pwmchip_add(&pdev->dev, &priv->chip);
    if (ret)
        return dev_err_probe(&pdev->dev, ret, "Failed to add PWM chip\n");

    platform_set_drvdata(pdev, priv);

    /* Create debugfs entry */
    priv->debugfs = debugfs_create_file("demo_pwm", 0444, NULL,
                                        priv, &demo_pwm_debugfs_fops);

    dev_info(&pdev->dev, "Demo PWM controller registered: %d channels\n",
             NUM_CHANNELS);

    return 0;
}

static void demo_pwm_remove(struct platform_device *pdev)
{
    struct demo_pwm *priv = platform_get_drvdata(pdev);

    debugfs_remove(priv->debugfs);
    dev_info(&pdev->dev, "Demo PWM controller removed\n");
}

static const struct of_device_id demo_pwm_of_match[] = {
    { .compatible = "demo,pwm-controller" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_pwm_of_match);

static struct platform_driver demo_pwm_driver = {
    .probe = demo_pwm_probe,
    .remove = demo_pwm_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = demo_pwm_of_match,
    },
};

/* Self-registering platform device for testing */
static struct platform_device *demo_pdev;

static int __init demo_pwm_init(void)
{
    int ret;

    ret = platform_driver_register(&demo_pwm_driver);
    if (ret)
        return ret;

    demo_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(demo_pdev)) {
        platform_driver_unregister(&demo_pwm_driver);
        return PTR_ERR(demo_pdev);
    }

    return 0;
}

static void __exit demo_pwm_exit(void)
{
    platform_device_unregister(demo_pdev);
    platform_driver_unregister(&demo_pwm_driver);
}

module_init(demo_pwm_init);
module_exit(demo_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Demo PWM Controller Driver");
