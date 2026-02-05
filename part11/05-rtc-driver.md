---
layout: default
title: "11.5 RTC Driver"
parent: "Part 11: IIO, RTC, Regulator, Clock Drivers"
nav_order: 5
---

# RTC Driver

This chapter covers implementing RTC drivers using the `rtc_device` structure and `rtc_class_ops`.

## Basic RTC Driver

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/regmap.h>
#include <linux/bcd.h>

/* Register definitions */
#define REG_SECONDS     0x00
#define REG_MINUTES     0x01
#define REG_HOURS       0x02
#define REG_DAY         0x03
#define REG_DATE        0x04
#define REG_MONTH       0x05
#define REG_YEAR        0x06
#define REG_CONTROL     0x07

#define CTRL_OUT        BIT(7)
#define CTRL_SQWE       BIT(4)
#define CTRL_RS1        BIT(1)
#define CTRL_RS0        BIT(0)

struct my_rtc {
    struct regmap *regmap;
    struct rtc_device *rtc;
};

static int my_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
    struct my_rtc *rtc = dev_get_drvdata(dev);
    u8 regs[7];
    int ret;

    ret = regmap_bulk_read(rtc->regmap, REG_SECONDS, regs, sizeof(regs));
    if (ret)
        return ret;

    tm->tm_sec = bcd2bin(regs[0] & 0x7F);
    tm->tm_min = bcd2bin(regs[1] & 0x7F);
    tm->tm_hour = bcd2bin(regs[2] & 0x3F);  /* 24-hour mode */
    tm->tm_wday = bcd2bin(regs[3] & 0x07) - 1;
    tm->tm_mday = bcd2bin(regs[4] & 0x3F);
    tm->tm_mon = bcd2bin(regs[5] & 0x1F) - 1;
    tm->tm_year = bcd2bin(regs[6]) + 100;   /* Years since 1900 */

    return 0;
}

static int my_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
    struct my_rtc *rtc = dev_get_drvdata(dev);
    u8 regs[7];

    regs[0] = bin2bcd(tm->tm_sec);
    regs[1] = bin2bcd(tm->tm_min);
    regs[2] = bin2bcd(tm->tm_hour);
    regs[3] = bin2bcd(tm->tm_wday + 1);
    regs[4] = bin2bcd(tm->tm_mday);
    regs[5] = bin2bcd(tm->tm_mon + 1);
    regs[6] = bin2bcd(tm->tm_year - 100);

    return regmap_bulk_write(rtc->regmap, REG_SECONDS, regs, sizeof(regs));
}

static const struct rtc_class_ops my_rtc_ops = {
    .read_time = my_rtc_read_time,
    .set_time = my_rtc_set_time,
};

static const struct regmap_config my_rtc_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = REG_CONTROL,
};

static int my_rtc_probe(struct i2c_client *client)
{
    struct my_rtc *rtc;
    int ret;

    rtc = devm_kzalloc(&client->dev, sizeof(*rtc), GFP_KERNEL);
    if (!rtc)
        return -ENOMEM;

    rtc->regmap = devm_regmap_init_i2c(client, &my_rtc_regmap_config);
    if (IS_ERR(rtc->regmap))
        return PTR_ERR(rtc->regmap);

    dev_set_drvdata(&client->dev, rtc);

    rtc->rtc = devm_rtc_device_register(&client->dev, "my-rtc",
                                        &my_rtc_ops, THIS_MODULE);
    if (IS_ERR(rtc->rtc))
        return PTR_ERR(rtc->rtc);

    return 0;
}

static const struct of_device_id my_rtc_of_match[] = {
    { .compatible = "vendor,my-rtc" },
    { }
};
MODULE_DEVICE_TABLE(of, my_rtc_of_match);

static struct i2c_driver my_rtc_driver = {
    .driver = {
        .name = "my-rtc",
        .of_match_table = my_rtc_of_match,
    },
    .probe = my_rtc_probe,
};
module_i2c_driver(my_rtc_driver);

MODULE_LICENSE("GPL");
```

## RTC with Alarm Support

```c
#define REG_ALARM_SEC   0x08
#define REG_ALARM_MIN   0x09
#define REG_ALARM_HOUR  0x0A
#define REG_ALARM_DAY   0x0B
#define REG_CONTROL     0x0E
#define REG_STATUS      0x0F

#define CTRL_AIE        BIT(0)  /* Alarm interrupt enable */
#define STATUS_AF       BIT(0)  /* Alarm flag */

struct my_rtc_alarm {
    struct regmap *regmap;
    struct rtc_device *rtc;
    int irq;
};

static int my_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    struct my_rtc_alarm *rtc = dev_get_drvdata(dev);
    u8 regs[4];
    unsigned int ctrl;
    int ret;

    ret = regmap_bulk_read(rtc->regmap, REG_ALARM_SEC, regs, sizeof(regs));
    if (ret)
        return ret;

    alrm->time.tm_sec = bcd2bin(regs[0] & 0x7F);
    alrm->time.tm_min = bcd2bin(regs[1] & 0x7F);
    alrm->time.tm_hour = bcd2bin(regs[2] & 0x3F);
    alrm->time.tm_mday = bcd2bin(regs[3] & 0x3F);

    ret = regmap_read(rtc->regmap, REG_CONTROL, &ctrl);
    if (ret)
        return ret;

    alrm->enabled = !!(ctrl & CTRL_AIE);

    return 0;
}

static int my_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    struct my_rtc_alarm *rtc = dev_get_drvdata(dev);
    u8 regs[4];
    int ret;

    /* Clear alarm flag first */
    ret = regmap_update_bits(rtc->regmap, REG_STATUS, STATUS_AF, 0);
    if (ret)
        return ret;

    regs[0] = bin2bcd(alrm->time.tm_sec);
    regs[1] = bin2bcd(alrm->time.tm_min);
    regs[2] = bin2bcd(alrm->time.tm_hour);
    regs[3] = bin2bcd(alrm->time.tm_mday);

    ret = regmap_bulk_write(rtc->regmap, REG_ALARM_SEC, regs, sizeof(regs));
    if (ret)
        return ret;

    /* Enable/disable alarm interrupt */
    return regmap_update_bits(rtc->regmap, REG_CONTROL, CTRL_AIE,
                              alrm->enabled ? CTRL_AIE : 0);
}

static int my_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
    struct my_rtc_alarm *rtc = dev_get_drvdata(dev);

    return regmap_update_bits(rtc->regmap, REG_CONTROL, CTRL_AIE,
                              enabled ? CTRL_AIE : 0);
}

static irqreturn_t my_rtc_irq_handler(int irq, void *data)
{
    struct my_rtc_alarm *rtc = data;
    unsigned int status;

    regmap_read(rtc->regmap, REG_STATUS, &status);

    if (status & STATUS_AF) {
        /* Clear alarm flag */
        regmap_update_bits(rtc->regmap, REG_STATUS, STATUS_AF, 0);

        /* Notify RTC core */
        rtc_update_irq(rtc->rtc, 1, RTC_AF | RTC_IRQF);

        return IRQ_HANDLED;
    }

    return IRQ_NONE;
}

static const struct rtc_class_ops my_rtc_alarm_ops = {
    .read_time = my_rtc_read_time,
    .set_time = my_rtc_set_time,
    .read_alarm = my_rtc_read_alarm,
    .set_alarm = my_rtc_set_alarm,
    .alarm_irq_enable = my_rtc_alarm_irq_enable,
};

static int my_rtc_alarm_probe(struct i2c_client *client)
{
    struct my_rtc_alarm *rtc;
    int ret;

    rtc = devm_kzalloc(&client->dev, sizeof(*rtc), GFP_KERNEL);
    if (!rtc)
        return -ENOMEM;

    rtc->regmap = devm_regmap_init_i2c(client, &my_rtc_regmap_config);
    if (IS_ERR(rtc->regmap))
        return PTR_ERR(rtc->regmap);

    rtc->irq = client->irq;

    dev_set_drvdata(&client->dev, rtc);

    /* Register RTC device */
    rtc->rtc = devm_rtc_allocate_device(&client->dev);
    if (IS_ERR(rtc->rtc))
        return PTR_ERR(rtc->rtc);

    rtc->rtc->ops = &my_rtc_alarm_ops;

    /* Set time range */
    rtc->rtc->range_min = RTC_TIMESTAMP_BEGIN_2000;
    rtc->rtc->range_max = RTC_TIMESTAMP_END_2099;

    /* Request IRQ if available */
    if (rtc->irq > 0) {
        ret = devm_request_threaded_irq(&client->dev, rtc->irq,
                                        NULL, my_rtc_irq_handler,
                                        IRQF_ONESHOT,
                                        "my-rtc-alarm", rtc);
        if (ret)
            return ret;

        /* Enable wakeup */
        device_init_wakeup(&client->dev, true);
    }

    return devm_rtc_register_device(rtc->rtc);
}
```

## Modern Registration API

```c
static int my_rtc_probe(struct i2c_client *client)
{
    struct my_rtc *rtc;

    rtc = devm_kzalloc(&client->dev, sizeof(*rtc), GFP_KERNEL);
    if (!rtc)
        return -ENOMEM;

    /* Allocate RTC device */
    rtc->rtc = devm_rtc_allocate_device(&client->dev);
    if (IS_ERR(rtc->rtc))
        return PTR_ERR(rtc->rtc);

    /* Configure RTC */
    rtc->rtc->ops = &my_rtc_ops;
    rtc->rtc->range_min = RTC_TIMESTAMP_BEGIN_2000;
    rtc->rtc->range_max = RTC_TIMESTAMP_END_2099;

    /* Set features */
    set_bit(RTC_FEATURE_ALARM, rtc->rtc->features);
    clear_bit(RTC_FEATURE_UPDATE_INTERRUPT, rtc->rtc->features);

    /* Register the device */
    return devm_rtc_register_device(rtc->rtc);
}
```

## Wakeup Source

```c
static int my_rtc_probe(struct i2c_client *client)
{
    /* ... */

    if (device_property_read_bool(&client->dev, "wakeup-source")) {
        device_init_wakeup(&client->dev, true);
        dev_pm_set_wake_irq(&client->dev, client->irq);
    }

    /* ... */
}

static int my_rtc_suspend(struct device *dev)
{
    struct my_rtc *rtc = dev_get_drvdata(dev);

    if (device_may_wakeup(dev))
        enable_irq_wake(rtc->irq);

    return 0;
}

static int my_rtc_resume(struct device *dev)
{
    struct my_rtc *rtc = dev_get_drvdata(dev);

    if (device_may_wakeup(dev))
        disable_irq_wake(rtc->irq);

    return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(my_rtc_pm_ops,
                                my_rtc_suspend, my_rtc_resume);
```

## Device Tree Binding

```dts
&i2c1 {
    rtc@68 {
        compatible = "vendor,my-rtc";
        reg = <0x68>;
        interrupt-parent = <&gpio>;
        interrupts = <5 IRQ_TYPE_LEVEL_LOW>;
        wakeup-source;
    };
};
```

## Summary

- Use `devm_rtc_allocate_device()` + `devm_rtc_register_device()`
- Implement `read_time()` and `set_time()` at minimum
- Add alarm support with `read_alarm()`, `set_alarm()`, `alarm_irq_enable()`
- Use `rtc_update_irq()` to notify core of interrupts
- Set `range_min` and `range_max` for time range validation
- Handle wakeup with `device_init_wakeup()`

## Further Reading

- [RTC Drivers](https://docs.kernel.org/driver-api/rtc.html) - Kernel documentation
- [DS3231 Driver](https://elixir.bootlin.com/linux/v6.6/source/drivers/rtc/rtc-ds1307.c) - Reference
- [PCF8523 Driver](https://elixir.bootlin.com/linux/v6.6/source/drivers/rtc/rtc-pcf8523.c) - Example

## Next

Learn about the [Regulator subsystem]({% link part11/06-regulator-subsystem.md %}).
