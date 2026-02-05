# GPIO Chip Demo Driver

This example demonstrates how to implement a GPIO provider (gpio_chip) as a platform driver. It creates a virtual GPIO controller with 8 pins that can be used for testing.

## Features

- gpio_chip implementation with full GPIO operations
- IRQ chip integration for interrupt support
- Debugfs interface for testing and debugging
- Self-registering platform device for easy testing
- Compatible with standard GPIO tools

## Prerequisites

```bash
# Ensure kernel headers are installed
sudo apt-get install linux-headers-$(uname -r)

# Optional: libgpiod for modern GPIO control
sudo apt-get install gpiod
```

## Building

```bash
make
```

## Loading and Testing

```bash
# Load the module
sudo insmod gpio_chip_demo.ko

# Run the test script
chmod +x test_gpio.sh
./test_gpio.sh
```

## Manual Testing

### Using sysfs (legacy interface)

```bash
# Find the GPIO chip base
cat /sys/class/gpio/gpiochip*/label
# Look for "demo-gpio"

# Get base number (e.g., 512)
GPIO_BASE=$(cat /sys/class/gpio/gpiochip*/base | head -1)

# Export a GPIO
echo $GPIO_BASE | sudo tee /sys/class/gpio/export

# Set direction
echo "out" | sudo tee /sys/class/gpio/gpio$GPIO_BASE/direction

# Set value
echo "1" | sudo tee /sys/class/gpio/gpio$GPIO_BASE/value

# Read value
cat /sys/class/gpio/gpio$GPIO_BASE/value

# Unexport
echo $GPIO_BASE | sudo tee /sys/class/gpio/unexport
```

### Using libgpiod (modern interface)

```bash
# List GPIO chips
gpiodetect

# Get info about demo GPIO chip
gpioinfo demo-gpio

# Read GPIO value
gpioget demo-gpio 0

# Set GPIO output
gpioset demo-gpio 0=1

# Monitor GPIO for events
gpiomon demo-gpio 0
```

### Using debugfs

```bash
# View GPIO controller status
sudo cat /sys/kernel/debug/demo_gpio

# Simulate input change (for input pins only)
# Format: "pin value"
echo "0 1" | sudo tee /sys/kernel/debug/demo_gpio

# View kernel GPIO debug info
sudo cat /sys/kernel/debug/gpio
```

## Device Tree Binding

```dts
demo_gpio: gpio-controller {
    compatible = "demo,gpio-controller";
    gpio-controller;
    #gpio-cells = <2>;
    interrupt-controller;
    #interrupt-cells = <2>;
};

/* Using the GPIOs */
example_device {
    compatible = "example,device";
    reset-gpios = <&demo_gpio 0 GPIO_ACTIVE_LOW>;
    enable-gpios = <&demo_gpio 1 GPIO_ACTIVE_HIGH>;
};
```

## Key Concepts Demonstrated

### gpio_chip Structure

```c
gpio->gc.label = DRIVER_NAME;
gpio->gc.parent = &pdev->dev;
gpio->gc.base = -1;              /* Dynamic allocation */
gpio->gc.ngpio = NUM_GPIOS;
gpio->gc.get_direction = demo_gpio_get_direction;
gpio->gc.direction_input = demo_gpio_direction_input;
gpio->gc.direction_output = demo_gpio_direction_output;
gpio->gc.get = demo_gpio_get;
gpio->gc.set = demo_gpio_set;
gpio->gc.set_multiple = demo_gpio_set_multiple;
```

### IRQ Chip Integration

```c
girq = &gpio->gc.irq;
gpio_irq_chip_set_chip(girq, &demo_irq_chip_template);
girq->handler = handle_simple_irq;
girq->default_type = IRQ_TYPE_NONE;
```

### Direction Control

```c
static int demo_gpio_direction_output(struct gpio_chip *gc,
                                      unsigned int offset, int value)
{
    struct demo_gpio *gpio = gpiochip_get_data(gc);

    spin_lock_irqsave(&gpio->lock, flags);
    /* Set value first, then direction */
    if (value)
        gpio->reg_data |= BIT(offset);
    else
        gpio->reg_data &= ~BIT(offset);
    gpio->reg_dir |= BIT(offset);
    spin_unlock_irqrestore(&gpio->lock, flags);

    return 0;
}
```

## Files

- `gpio_chip_demo.c` - Main driver source
- `Makefile` - Build system
- `test_gpio.sh` - Test script
- `README.md` - This file

## Virtual Registers

The driver simulates hardware registers:

| Register | Offset | Description |
|----------|--------|-------------|
| DATA     | 0x00   | GPIO values |
| DIR      | 0x04   | Direction (1=output) |
| IRQ_EN   | 0x08   | IRQ enable mask |
| IRQ_TYPE | 0x0C   | IRQ type (edge/level) |
| IRQ_POL  | 0x10   | IRQ polarity |
| IRQ_STATUS | 0x14 | Pending IRQs |

## Related Documentation

- Part 9.6: GPIO Consumer API
- Part 9.7: GPIO Provider (gpio_chip)
