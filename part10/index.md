---
layout: default
title: "Part 10: PWM, Watchdog, HWMON, LED Drivers"
nav_order: 10
has_children: true
---

# Part 10: PWM, Watchdog, HWMON, LED Drivers

This part covers several specialized device subsystems commonly used in embedded Linux systems. Each subsystem provides a standardized interface for specific types of hardware.

```mermaid
flowchart TB
    subgraph Subsystems["Linux Device Subsystems"]
        PWM["PWM<br/>Pulse Width Modulation"]
        WDT["Watchdog<br/>System Monitoring"]
        HWMON["HWMON<br/>Hardware Monitoring"]
        LED["LED<br/>LED Class"]
    end

    subgraph Interfaces["User Interfaces"]
        PWMFS["/sys/class/pwm/"]
        WDTDEV["/dev/watchdog"]
        HWMONFS["/sys/class/hwmon/"]
        LEDFS["/sys/class/leds/"]
    end

    subgraph Applications["Applications"]
        FAN["Fan Control"]
        MOTOR["Motor Control"]
        BACKLIGHT["Backlight"]
        SAFETY["Safety Monitor"]
        SENSOR["Sensor Monitoring"]
        STATUS["Status Indicators"]
    end

    PWM --> PWMFS
    WDT --> WDTDEV
    HWMON --> HWMONFS
    LED --> LEDFS

    PWMFS --> FAN
    PWMFS --> MOTOR
    PWMFS --> BACKLIGHT
    WDTDEV --> SAFETY
    HWMONFS --> SENSOR
    LEDFS --> STATUS

    style PWM fill:#738f99,stroke:#0277bd
    style WDT fill:#8f7370,stroke:#c62828
    style HWMON fill:#7a8f73,stroke:#2e7d32
    style LED fill:#8f8a73,stroke:#f9a825
```

## What You'll Learn

### PWM Subsystem
- PWM chip architecture and implementation
- PWM consumer API
- Duty cycle and period control
- Polarity configuration

### Watchdog Drivers
- Watchdog architecture
- `watchdog_device` implementation
- Timeout configuration
- Pretimeout support

### Hardware Monitoring (HWMON)
- HWMON subsystem architecture
- Temperature, voltage, fan speed monitoring
- `hwmon_chip_info` registration
- Sensor attributes

### LED Class Drivers
- LED class framework
- Brightness control
- LED triggers
- Hardware blink support

## Subsystem Overview

```mermaid
flowchart LR
    subgraph PWM_Sub["PWM Subsystem"]
        PWMCHIP["pwm_chip"]
        PWMDEV["pwm_device"]
        PWMOPS["pwm_ops"]
        PWMCHIP --> PWMOPS
        PWMCHIP --> PWMDEV
    end

    subgraph WDT_Sub["Watchdog Subsystem"]
        WDTDEV2["watchdog_device"]
        WDTOPS["watchdog_ops"]
        WDTINFO["watchdog_info"]
        WDTDEV2 --> WDTOPS
        WDTDEV2 --> WDTINFO
    end

    subgraph HWMON_Sub["HWMON Subsystem"]
        HWDEV["hwmon_device"]
        HWCHIP["hwmon_chip_info"]
        HWCHAN["hwmon_channel_info"]
        HWDEV --> HWCHIP
        HWCHIP --> HWCHAN
    end

    subgraph LED_Sub["LED Subsystem"]
        LEDCDEV["led_classdev"]
        LEDOPS["brightness_set"]
        TRIGGER["led_trigger"]
        LEDCDEV --> LEDOPS
        LEDCDEV --> TRIGGER
    end

    style PWM_Sub fill:#5f7a82,stroke:#0277bd
    style WDT_Sub fill:#826563,stroke:#c62828
    style HWMON_Sub fill:#5f8259,stroke:#2e7d32
    style LED_Sub fill:#827f5f,stroke:#f9a825
```

## Key Structures

| Subsystem | Main Structure | Registration Function |
|-----------|---------------|----------------------|
| PWM | `struct pwm_chip` | `pwmchip_add()` |
| Watchdog | `struct watchdog_device` | `watchdog_register_device()` |
| HWMON | `struct hwmon_chip_info` | `hwmon_device_register_with_info()` |
| LED | `struct led_classdev` | `led_classdev_register()` |

## Common Patterns

All these subsystems share common patterns:

1. **Provider/Consumer Model**: Drivers implement providers; other drivers/userspace consume
2. **sysfs Interface**: Standard attributes exposed in `/sys/class/`
3. **Device Tree Integration**: Properties for configuration
4. **Managed Resources**: `devm_*` variants for automatic cleanup

## In This Part

1. [PWM Subsystem]({% link part10/01-pwm-subsystem.md %}) - PWM architecture overview
2. [PWM Provider]({% link part10/02-pwm-provider.md %}) - Implementing pwm_chip
3. [Watchdog Subsystem]({% link part10/03-watchdog-subsystem.md %}) - Watchdog architecture
4. [Watchdog Driver]({% link part10/04-watchdog-driver.md %}) - Implementing watchdog_device
5. [HWMON Subsystem]({% link part10/05-hwmon-subsystem.md %}) - Hardware monitoring overview
6. [HWMON Driver]({% link part10/06-hwmon-driver.md %}) - Implementing HWMON sensors
7. [LED Subsystem]({% link part10/07-led-subsystem.md %}) - LED class drivers

## Further Reading

- [PWM Documentation](https://docs.kernel.org/driver-api/pwm.html) - PWM subsystem
- [Watchdog API](https://docs.kernel.org/watchdog/index.html) - Watchdog drivers
- [HWMON Documentation](https://docs.kernel.org/hwmon/index.html) - Hardware monitoring
- [LED Class Documentation](https://docs.kernel.org/leds/index.html) - LED framework

## Prerequisites

Before starting this part, you should understand:
- Platform drivers and Device Tree (Part 8)
- Basic driver probe/remove lifecycle (Part 6)
- [Managed resources (devm_*)]({% link part6/05-devres.md %}) - all examples use device-managed allocation
- sysfs attributes (Part 6)
