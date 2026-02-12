---
layout: default
title: "Part 9: I2C, SPI, and GPIO Drivers"
nav_order: 9
has_children: true
---

# Part 9: I2C, SPI, and GPIO Drivers

I2C and SPI are the most common buses for connecting sensors, displays, and peripherals in embedded systems. This part covers writing drivers for devices on these buses.

## Bus Overview

```mermaid
flowchart TB
    subgraph I2C["I2C Bus"]
        direction LR
        I2C_CTRL[I2C Controller]
        I2C_D1[Device 0x48]
        I2C_D2[Device 0x50]
        I2C_CTRL --- I2C_D1
        I2C_CTRL --- I2C_D2
    end

    subgraph SPI["SPI Bus"]
        direction LR
        SPI_CTRL[SPI Controller]
        SPI_D1[Device CS0]
        SPI_D2[Device CS1]
        SPI_CTRL --- SPI_D1
        SPI_CTRL --- SPI_D2
    end

    subgraph GPIO["GPIO"]
        direction LR
        GPIO_CTRL[GPIO Controller]
        GPIO_P1[Pin 0]
        GPIO_P2[Pin 1]
        GPIO_CTRL --- GPIO_P1
        GPIO_CTRL --- GPIO_P2
    end

    CPU[CPU/SoC]
    CPU --> I2C
    CPU --> SPI
    CPU --> GPIO

    style I2C fill:#738f99,stroke:#0277bd
    style SPI fill:#8f8a73,stroke:#f9a825
    style GPIO fill:#7a8f73,stroke:#2e7d32
```

## Chapter Contents

| Chapter | Topic | Key Concepts |
|---------|-------|--------------|
| [9.1]({% link part9/01-i2c-subsystem.md %}) | I2C Subsystem | Bus architecture, adapters, clients |
| [9.2]({% link part9/02-i2c-client-driver.md %}) | I2C Client Driver | i2c_driver, probe/remove |
| [9.3]({% link part9/03-i2c-regmap.md %}) | I2C with Regmap | regmap abstraction for I2C |
| [9.4]({% link part9/04-spi-subsystem.md %}) | SPI Subsystem | Bus architecture, transfers |
| [9.5]({% link part9/05-spi-driver.md %}) | SPI Driver | spi_driver, SPI messaging |
| [9.6]({% link part9/06-gpio-consumer.md %}) | GPIO Consumer API | gpiod_get, set/get values |
| [9.7]({% link part9/07-gpio-provider.md %}) | GPIO Provider | gpio_chip implementation |
| [9.8]({% link part9/08-pinctrl.md %}) | Pin Control | Pin muxing, pin configuration |

## I2C vs SPI Comparison

| Feature | I2C | SPI |
|---------|-----|-----|
| Wires | 2 (SDA, SCL) | 4+ (MOSI, MISO, CLK, CS) |
| Addressing | 7-bit address | Chip select lines |
| Speed | Up to 3.4 MHz | Up to 100+ MHz |
| Topology | Multi-master, multi-slave | Single master, multi-slave |
| Complexity | Higher protocol | Simple |
| Use case | Sensors, EEPROMs | Flash, displays, high-speed |

## I2C Architecture

```mermaid
flowchart TB
    subgraph UserSpace["User Space"]
        APP[Application]
    end

    subgraph Kernel["Kernel"]
        I2CCORE[I2C Core]
        I2CDEV[i2c-dev]
        I2CADAPTER[I2C Adapter Driver]
        I2CCLIENT[I2C Client Driver]
    end

    subgraph Hardware
        CTRL[I2C Controller]
        DEV[I2C Device]
    end

    APP -->|"/dev/i2c-N"| I2CDEV
    I2CDEV --> I2CCORE
    I2CCLIENT --> I2CCORE
    I2CCORE --> I2CADAPTER
    I2CADAPTER --> CTRL
    CTRL -->|"I2C Bus"| DEV
```

## SPI Architecture

```mermaid
flowchart TB
    subgraph UserSpace["User Space"]
        APP[Application]
    end

    subgraph Kernel["Kernel"]
        SPICORE[SPI Core]
        SPIDEV[spidev]
        SPICTRL[SPI Controller Driver]
        SPICLIENT[SPI Device Driver]
    end

    subgraph Hardware
        CTRL[SPI Controller]
        DEV[SPI Device]
    end

    APP -->|"/dev/spidevN.M"| SPIDEV
    SPIDEV --> SPICORE
    SPICLIENT --> SPICORE
    SPICORE --> SPICTRL
    SPICTRL --> CTRL
    CTRL -->|"SPI Bus"| DEV
```

## Key Data Structures

### I2C

```c
/* I2C client (device) */
struct i2c_client {
    unsigned short addr;        /* 7-bit I2C address */
    char name[I2C_NAME_SIZE];   /* Device name */
    struct i2c_adapter *adapter; /* Parent adapter */
    struct device dev;          /* Device structure */
};

/* I2C driver */
struct i2c_driver {
    int (*probe)(struct i2c_client *client);
    void (*remove)(struct i2c_client *client);
    struct device_driver driver;
    const struct i2c_device_id *id_table;
};
```

### SPI

```c
/* SPI device */
struct spi_device {
    struct device dev;
    struct spi_controller *controller;
    u32 max_speed_hz;
    u8 chip_select;
    u8 bits_per_word;
    u32 mode;  /* SPI_MODE_0..3, SPI_CS_HIGH, etc. */
};

/* SPI driver */
struct spi_driver {
    int (*probe)(struct spi_device *spi);
    void (*remove)(struct spi_device *spi);
    struct device_driver driver;
    const struct spi_device_id *id_table;
};
```

## Examples

This part includes working examples:

- **i2c-sensor**: I2C temperature sensor driver
- **spi-device**: SPI device driver with regmap
- **gpio-driver**: GPIO controller (gpio_chip)

## Prerequisites

Before starting this part, ensure you understand:

- Platform drivers (Part 6)
- [Managed resources (devm_*)]({% link part6/05-devres.md %}) - used extensively in these drivers
- Device Tree (Part 8)
- Interrupt handling (Part 7)

## Further Reading

- [I2C Documentation](https://docs.kernel.org/i2c/index.html) - I2C subsystem guide
- [SPI Documentation](https://docs.kernel.org/spi/index.html) - SPI subsystem guide
- [GPIO Documentation](https://docs.kernel.org/driver-api/gpio/index.html) - GPIO API guide

## Next

Start with [I2C Subsystem]({% link part9/01-i2c-subsystem.md %}) to understand I2C bus architecture.
