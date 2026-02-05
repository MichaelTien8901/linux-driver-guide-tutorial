---
layout: default
title: "16.3 Wakeup Sources"
parent: "Part 16: Power Management"
nav_order: 3
---

# Wakeup Sources

This chapter covers wakeup sources: how devices can wake the system from suspend.

## Wakeup Capability

A wakeup source is a device that can bring the system out of suspend:

```mermaid
flowchart LR
    A["System Suspended"] -->|"IRQ from<br/>wakeup device"| B["System Wakes"]
    B --> C["Resume Callbacks"]
    C --> D["System Running"]

    style A fill:#826563,stroke:#c62828
    style D fill:#7a8f73,stroke:#2e7d32
```

**Common wakeup sources:**
- Power button
- Keyboard/mouse
- Network (Wake-on-LAN)
- RTC alarm
- USB devices

## Enabling Wakeup Capability

In probe, declare that your device can wake the system:

```c
static int my_probe(struct platform_device *pdev)
{
    struct my_dev *dev;
    int ret;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* Get IRQ */
    dev->irq = platform_get_irq(pdev, 0);
    if (dev->irq < 0)
        return dev->irq;

    /* Register IRQ handler */
    ret = devm_request_irq(&pdev->dev, dev->irq, my_irq_handler,
                            0, "my_device", dev);
    if (ret)
        return ret;

    /* Enable wakeup capability (user can enable/disable via sysfs) */
    device_init_wakeup(&pdev->dev, true);

    platform_set_drvdata(pdev, dev);
    return 0;
}

static void my_remove(struct platform_device *pdev)
{
    device_init_wakeup(&pdev->dev, false);
}
```

## Suspend with Wakeup

Enable the IRQ as a wakeup source during suspend:

```c
static int my_suspend(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    /* Check if user enabled wakeup for this device */
    if (device_may_wakeup(dev)) {
        /* Enable this IRQ to wake the system */
        enable_irq_wake(mydev->irq);
    }

    /* Normal suspend operations */
    disable_device_activity(mydev);

    return 0;
}

static int my_resume(struct device *dev)
{
    struct my_dev *mydev = dev_get_drvdata(dev);

    if (device_may_wakeup(dev)) {
        /* Disable wakeup - back to normal operation */
        disable_irq_wake(mydev->irq);
    }

    /* Normal resume operations */
    enable_device_activity(mydev);

    return 0;
}
```

## Wakeup Events

Track what caused the wakeup:

```c
static irqreturn_t my_irq_handler(int irq, void *data)
{
    struct my_dev *dev = data;

    /* Check if we're waking from suspend */
    if (pm_runtime_suspended(dev->dev)) {
        /* Report this as a wakeup event */
        pm_wakeup_event(dev->dev, 0);
    }

    /* Handle interrupt normally... */
    return IRQ_HANDLED;
}
```

## Wakeup Event with Timeout

Prevent system from suspending while processing wakeup:

```c
static irqreturn_t button_irq_handler(int irq, void *data)
{
    struct button_dev *dev = data;

    /* Keep system awake for 1000ms to process event */
    pm_wakeup_event(dev->dev, 1000);

    /* Queue work to handle button press */
    schedule_work(&dev->button_work);

    return IRQ_HANDLED;
}
```

## Wakeup Source API

For more control, use wakeup source objects:

```c
struct my_dev {
    struct device *dev;
    struct wakeup_source *ws;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_dev *dev;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->dev = &pdev->dev;

    /* Create wakeup source */
    dev->ws = wakeup_source_register(dev->dev, "my_device_wakeup");
    if (!dev->ws)
        return -ENOMEM;

    return 0;
}

static void my_remove(struct platform_device *pdev)
{
    struct my_dev *dev = platform_get_drvdata(pdev);

    wakeup_source_unregister(dev->ws);
}

/* In your event handling code */
static void handle_event(struct my_dev *dev)
{
    /* Prevent suspend while handling event */
    __pm_stay_awake(dev->ws);

    /* ... process event ... */

    /* Allow suspend again */
    __pm_relax(dev->ws);
}

/* Or with timeout */
static void handle_event_with_timeout(struct my_dev *dev)
{
    /* Stay awake for 500ms */
    __pm_wakeup_event(dev->ws, 500);

    /* Process event... */
}
```

## Device Tree Wakeup

Configure wakeup in device tree:

```dts
button {
    compatible = "my,button";
    interrupt-parent = <&gpio>;
    interrupts = <5 IRQ_TYPE_EDGE_FALLING>;
    wakeup-source;  /* Mark as wakeup capable */
};
```

Parse in driver:

```c
static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;

    /* Check if marked as wakeup source in DT */
    if (device_property_read_bool(dev, "wakeup-source")) {
        device_init_wakeup(dev, true);
    }

    return 0;
}
```

## GPIO Wakeup Example

Complete example for a GPIO button wakeup source:

```c
struct button_dev {
    struct device *dev;
    int gpio;
    int irq;
    struct wakeup_source *ws;
};

static irqreturn_t button_isr(int irq, void *data)
{
    struct button_dev *btn = data;

    /* Keep system awake briefly */
    pm_wakeup_event(btn->dev, 200);

    /* Report button event... */
    return IRQ_HANDLED;
}

static int button_probe(struct platform_device *pdev)
{
    struct button_dev *btn;
    int ret;

    btn = devm_kzalloc(&pdev->dev, sizeof(*btn), GFP_KERNEL);
    if (!btn)
        return -ENOMEM;

    btn->dev = &pdev->dev;

    /* Get GPIO from DT */
    btn->gpio = of_get_named_gpio(pdev->dev.of_node, "gpios", 0);
    if (btn->gpio < 0)
        return btn->gpio;

    ret = devm_gpio_request_one(&pdev->dev, btn->gpio,
                                 GPIOF_IN, "button");
    if (ret)
        return ret;

    btn->irq = gpio_to_irq(btn->gpio);

    ret = devm_request_irq(&pdev->dev, btn->irq, button_isr,
                            IRQF_TRIGGER_FALLING, "button", btn);
    if (ret)
        return ret;

    /* Enable as wakeup source */
    device_init_wakeup(&pdev->dev, true);

    platform_set_drvdata(pdev, btn);
    return 0;
}

static int button_suspend(struct device *dev)
{
    struct button_dev *btn = dev_get_drvdata(dev);

    if (device_may_wakeup(dev))
        enable_irq_wake(btn->irq);

    return 0;
}

static int button_resume(struct device *dev)
{
    struct button_dev *btn = dev_get_drvdata(dev);

    if (device_may_wakeup(dev))
        disable_irq_wake(btn->irq);

    return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(button_pm_ops, button_suspend, button_resume);

static struct platform_driver button_driver = {
    .probe = button_probe,
    .driver = {
        .name = "my_button",
        .pm = pm_sleep_ptr(&button_pm_ops),
    },
};
```

## Sysfs Interface

Users control wakeup via sysfs:

```bash
# Check if device can wake system
cat /sys/devices/.../power/wakeup
# Returns: enabled, disabled, or empty (not capable)

# Enable wakeup
echo enabled > /sys/devices/.../power/wakeup

# Disable wakeup
echo disabled > /sys/devices/.../power/wakeup

# View wakeup statistics
cat /sys/devices/.../power/wakeup_count
cat /sys/devices/.../power/wakeup_active_count
```

## Summary

| Function | Purpose |
|----------|---------|
| `device_init_wakeup()` | Enable wakeup capability |
| `device_may_wakeup()` | Check if user enabled wakeup |
| `enable_irq_wake()` | Mark IRQ as wakeup source |
| `disable_irq_wake()` | Remove IRQ wakeup marking |
| `pm_wakeup_event()` | Report wakeup event |
| `__pm_stay_awake()` | Prevent suspend |
| `__pm_relax()` | Allow suspend again |

**Best practices:**
1. Always use `device_may_wakeup()` to check user preference
2. Match `enable_irq_wake` with `disable_irq_wake`
3. Use `pm_wakeup_event` with timeout for event processing
4. Parse `wakeup-source` from device tree when appropriate

## Further Reading

- [Wakeup Sources](https://docs.kernel.org/driver-api/pm/wakeup.html) - Framework documentation
- [Device PM](https://docs.kernel.org/driver-api/pm/devices.html) - Power management overview
