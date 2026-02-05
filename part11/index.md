---
layout: default
title: "Part 11: IIO, RTC, Regulator, Clock Drivers"
nav_order: 11
has_children: true
---

# Part 11: IIO, RTC, Regulator, Clock Drivers

This part covers four important kernel subsystems: Industrial I/O (IIO) for sensors and data acquisition, Real-Time Clock (RTC) for timekeeping, Regulator framework for power management, and Common Clock Framework (CCF) for clock providers.

```mermaid
flowchart TB
    subgraph Subsystems["Kernel Subsystems"]
        IIO["IIO<br/>Industrial I/O"]
        RTC["RTC<br/>Real-Time Clock"]
        REG["Regulator<br/>Framework"]
        CLK["Clock<br/>Framework"]
    end

    subgraph Devices["Device Types"]
        ADC["ADC/DAC"]
        ACCEL["Accelerometers"]
        GYRO["Gyroscopes"]
        RTCDEV["RTC Chips"]
        PMIC["PMICs"]
        LDO["LDOs/Bucks"]
        PLL["PLLs"]
        CLKGEN["Clock Generators"]
    end

    subgraph Consumers["Consumers"]
        APPS["Applications"]
        THERMAL["Thermal"]
        DRIVERS["Other Drivers"]
    end

    ADC --> IIO
    ACCEL --> IIO
    GYRO --> IIO
    RTCDEV --> RTC
    PMIC --> REG
    LDO --> REG
    PLL --> CLK
    CLKGEN --> CLK

    IIO --> APPS
    RTC --> APPS
    REG --> DRIVERS
    CLK --> DRIVERS

    style Subsystems fill:#738f99,stroke:#0277bd
    style Devices fill:#7a8f73,stroke:#2e7d32
    style Consumers fill:#8f8a73,stroke:#f9a825
```

## What You'll Learn

### Industrial I/O (IIO)
- IIO subsystem architecture
- Channels, attributes, and buffers
- Triggered buffer acquisition
- IIO device registration

### Real-Time Clock (RTC)
- RTC subsystem overview
- `rtc_device` implementation
- Alarm and wake-up support
- RTC class operations

### Regulator Framework
- Voltage regulator concepts
- Provider (supply) drivers
- Consumer API
- Constraints and coupling

### Common Clock Framework
- Clock tree hierarchy
- `clk_hw` implementation
- Clock types (fixed, divider, mux, PLL)
- Clock operations

## Subsystem Overview

```mermaid
flowchart LR
    subgraph IIO_Sub["IIO Subsystem"]
        IIODEV["iio_dev"]
        IIOCHAN["iio_chan_spec"]
        IIOBUF["IIO Buffer"]
        IIOTRIG["IIO Trigger"]
    end

    subgraph RTC_Sub["RTC Subsystem"]
        RTCDEV2["rtc_device"]
        RTCOPS["rtc_class_ops"]
        RTCALARM["Alarms"]
    end

    subgraph REG_Sub["Regulator Framework"]
        REGDEV["regulator_dev"]
        REGDESC["regulator_desc"]
        REGCON["regulator_consumer"]
    end

    subgraph CLK_Sub["Clock Framework"]
        CLKHW["clk_hw"]
        CLKOPS["clk_ops"]
        CLKPROV["Clock Provider"]
    end

    style IIO_Sub fill:#5f7a82,stroke:#0277bd
    style RTC_Sub fill:#5f8259,stroke:#2e7d32
    style REG_Sub fill:#827f5f,stroke:#f9a825
    style CLK_Sub fill:#826563,stroke:#c62828
```

## Key Structures

| Subsystem | Main Structure | Registration Function |
|-----------|---------------|----------------------|
| IIO | `struct iio_dev` | `devm_iio_device_register()` |
| RTC | `struct rtc_device` | `devm_rtc_device_register()` |
| Regulator | `struct regulator_dev` | `devm_regulator_register()` |
| Clock | `struct clk_hw` | `devm_clk_hw_register()` |

## Common Patterns

These subsystems share several design patterns:

1. **Provider/Consumer Model**: Drivers expose resources; others consume them
2. **Hierarchical Organization**: Clocks in trees, regulators in supplies
3. **Device Tree Integration**: Standard bindings for configuration
4. **Managed Resources**: `devm_*` functions for automatic cleanup
5. **Runtime PM Integration**: Power-aware operation

## In This Part

1. [IIO Subsystem]({% link part11/01-iio-subsystem.md %}) - IIO architecture overview
2. [IIO Driver]({% link part11/02-iio-driver.md %}) - Implementing iio_dev
3. [IIO Buffers]({% link part11/03-iio-buffers.md %}) - Triggered buffer acquisition
4. [RTC Subsystem]({% link part11/04-rtc-subsystem.md %}) - RTC architecture
5. [RTC Driver]({% link part11/05-rtc-driver.md %}) - Implementing rtc_device
6. [Regulator Subsystem]({% link part11/06-regulator-subsystem.md %}) - Regulator framework
7. [Regulator Driver]({% link part11/07-regulator-driver.md %}) - Provider/consumer implementation
8. [Clock Framework]({% link part11/08-clock-framework.md %}) - CCF overview
9. [Clock Provider]({% link part11/09-clock-provider.md %}) - Implementing clk_hw

## Further Reading

- [IIO Documentation](https://docs.kernel.org/driver-api/iio/index.html) - Industrial I/O
- [RTC Documentation](https://docs.kernel.org/driver-api/rtc.html) - Real-time clock
- [Regulator Documentation](https://docs.kernel.org/power/regulator/index.html) - Regulator framework
- [Clock Framework](https://docs.kernel.org/driver-api/clk.html) - Common clock framework

## Prerequisites

Before starting this part, you should understand:
- Platform drivers and Device Tree (Part 8)
- I2C/SPI drivers (Part 9)
- [Managed resources (devm_*)]({% link part6/05-devres.md %}) - essential for these drivers
- Basic kernel memory management (Part 5)
