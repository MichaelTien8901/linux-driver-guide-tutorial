// SPDX-License-Identifier: GPL-2.0
/*
 * LED Class Demo Driver
 *
 * Demonstrates implementing LED class drivers with brightness control,
 * hardware blink support, and custom triggers.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/timer.h>

#define DRIVER_NAME     "demo-leds"
#define NUM_LEDS        3

struct demo_led {
    struct led_classdev cdev;
    struct timer_list blink_timer;
    unsigned long delay_on;
    unsigned long delay_off;
    bool blink_state;
    bool hw_blink_active;
    int brightness;
};

struct demo_leds {
    struct demo_led leds[NUM_LEDS];
};

/* Simulated hardware blink timer */
static void demo_led_blink_timer(struct timer_list *t)
{
    struct demo_led *led = from_timer(led, t, blink_timer);
    unsigned long delay;

    if (!led->hw_blink_active)
        return;

    led->blink_state = !led->blink_state;

    /* Simulate LED state change */
    pr_debug("LED %s: %s\n", led->cdev.name,
             led->blink_state ? "ON" : "OFF");

    delay = led->blink_state ? led->delay_on : led->delay_off;
    mod_timer(&led->blink_timer, jiffies + msecs_to_jiffies(delay));
}

static void demo_led_brightness_set(struct led_classdev *cdev,
                                    enum led_brightness brightness)
{
    struct demo_led *led = container_of(cdev, struct demo_led, cdev);

    /* Stop hardware blink if setting brightness directly */
    if (led->hw_blink_active) {
        led->hw_blink_active = false;
        del_timer_sync(&led->blink_timer);
    }

    led->brightness = brightness;

    /* Simulate setting LED brightness */
    pr_info("LED %s: brightness=%d/%d\n", cdev->name,
            brightness, cdev->max_brightness);
}

static enum led_brightness demo_led_brightness_get(struct led_classdev *cdev)
{
    struct demo_led *led = container_of(cdev, struct demo_led, cdev);
    return led->brightness;
}

static int demo_led_blink_set(struct led_classdev *cdev,
                              unsigned long *delay_on,
                              unsigned long *delay_off)
{
    struct demo_led *led = container_of(cdev, struct demo_led, cdev);

    /* If delays are 0, use defaults */
    if (*delay_on == 0 && *delay_off == 0) {
        *delay_on = 500;
        *delay_off = 500;
    }

    /* Minimum delay check (simulated hardware limitation) */
    if (*delay_on < 50 || *delay_off < 50) {
        pr_debug("LED %s: delay too short, using software blink\n",
                 cdev->name);
        return -EINVAL;  /* Fall back to software blink */
    }

    /* Configure hardware blink */
    led->delay_on = *delay_on;
    led->delay_off = *delay_off;
    led->blink_state = true;
    led->hw_blink_active = true;

    /* Start blink timer */
    mod_timer(&led->blink_timer,
              jiffies + msecs_to_jiffies(led->delay_on));

    pr_info("LED %s: hardware blink on=%lu off=%lu ms\n",
            cdev->name, *delay_on, *delay_off);

    return 0;
}

static int demo_leds_probe(struct platform_device *pdev)
{
    struct demo_leds *priv;
    int ret, i;
    const char *led_names[] = { "demo:red:status", "demo:green:power",
                                "demo:blue:activity" };
    const char *default_triggers[] = { "heartbeat", "default-on", "none" };

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    for (i = 0; i < NUM_LEDS; i++) {
        struct demo_led *led = &priv->leds[i];

        timer_setup(&led->blink_timer, demo_led_blink_timer, 0);

        led->cdev.name = led_names[i];
        led->cdev.brightness_set = demo_led_brightness_set;
        led->cdev.brightness_get = demo_led_brightness_get;
        led->cdev.blink_set = demo_led_blink_set;
        led->cdev.max_brightness = 255;
        led->cdev.default_trigger = default_triggers[i];

        ret = devm_led_classdev_register(&pdev->dev, &led->cdev);
        if (ret) {
            dev_err(&pdev->dev, "Failed to register LED %s: %d\n",
                    led_names[i], ret);
            return ret;
        }
    }

    platform_set_drvdata(pdev, priv);

    dev_info(&pdev->dev, "Demo LEDs registered: %d LEDs\n", NUM_LEDS);

    return 0;
}

static void demo_leds_remove(struct platform_device *pdev)
{
    struct demo_leds *priv = platform_get_drvdata(pdev);
    int i;

    for (i = 0; i < NUM_LEDS; i++) {
        struct demo_led *led = &priv->leds[i];
        led->hw_blink_active = false;
        del_timer_sync(&led->blink_timer);
    }

    dev_info(&pdev->dev, "Demo LEDs removed\n");
}

static const struct of_device_id demo_leds_of_match[] = {
    { .compatible = "demo,leds" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_leds_of_match);

static struct platform_driver demo_leds_driver = {
    .probe = demo_leds_probe,
    .remove = demo_leds_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = demo_leds_of_match,
    },
};

/* Self-registering platform device */
static struct platform_device *demo_pdev;

static int __init demo_leds_init(void)
{
    int ret;

    ret = platform_driver_register(&demo_leds_driver);
    if (ret)
        return ret;

    demo_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(demo_pdev)) {
        platform_driver_unregister(&demo_leds_driver);
        return PTR_ERR(demo_pdev);
    }

    return 0;
}

static void __exit demo_leds_exit(void)
{
    platform_device_unregister(demo_pdev);
    platform_driver_unregister(&demo_leds_driver);
}

module_init(demo_leds_init);
module_exit(demo_leds_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Demo LED Class Driver");
