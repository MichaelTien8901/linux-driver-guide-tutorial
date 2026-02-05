# SPI Flash Demo Driver

This example demonstrates how to write an SPI device driver for a flash memory device with character device interface.

## Features

- SPI device driver using `module_spi_driver()`
- Flash-like operations (read, write, erase)
- Character device interface via miscdevice
- Page-aligned programming with automatic boundary handling
- ioctl support for erase operations

## Prerequisites

```bash
# Ensure kernel headers are installed
sudo apt-get install linux-headers-$(uname -r)
```

## Building

```bash
make
```

## Device Tree Binding

```dts
&spi0 {
    status = "okay";

    flash@0 {
        compatible = "demo,spi-flash";
        reg = <0>;                      /* CS0 */
        spi-max-frequency = <10000000>; /* 10 MHz */
    };
};
```

## Testing Without Hardware

The driver includes an internal buffer for simulation, allowing testing without actual SPI flash hardware:

```bash
# Build the module
make

# Check module info
modinfo spi_flash_demo.ko

# Note: Without actual SPI hardware configured, the driver will load
# but won't have a device to bind to. On development boards with SPI:

# Load the module
sudo insmod spi_flash_demo.ko

# Check for the device
ls -la /dev/demo_flash

# Read from flash
dd if=/dev/demo_flash bs=256 count=1 | hexdump -C

# Write to flash
echo "Hello Flash!" | sudo dd of=/dev/demo_flash bs=256 conv=notrunc

# Verify
dd if=/dev/demo_flash bs=256 count=1 | hexdump -C

# Unload
sudo rmmod spi_flash_demo
```

## ioctl Commands

The driver supports ioctl commands for flash operations:

```c
#include <sys/ioctl.h>

#define FLASH_IOC_MAGIC     'F'
#define FLASH_IOC_ERASE     _IOW(FLASH_IOC_MAGIC, 1, u32)
#define FLASH_IOC_GETSIZE   _IOR(FLASH_IOC_MAGIC, 2, u32)

// Erase sector at address 0x1000
u32 addr = 0x1000;
ioctl(fd, FLASH_IOC_ERASE, &addr);

// Get flash size
u32 size;
ioctl(fd, FLASH_IOC_GETSIZE, &size);
```

## Key Concepts Demonstrated

### SPI Transfer Structure

```c
static int demo_flash_read(struct demo_flash *flash, u32 addr,
                           u8 *buf, size_t len)
{
    struct spi_transfer t[2];
    struct spi_message m;
    u8 cmd[4];

    /* Command + 24-bit address */
    cmd[0] = CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    memset(t, 0, sizeof(t));
    t[0].tx_buf = cmd;
    t[0].len = 4;
    t[1].rx_buf = buf;
    t[1].len = len;

    spi_message_init(&m);
    spi_message_add_tail(&t[0], &m);
    spi_message_add_tail(&t[1], &m);

    return spi_sync(flash->spi, &m);
}
```

### SPI Mode Configuration

```c
static int demo_flash_probe(struct spi_device *spi)
{
    spi->mode = SPI_MODE_0;
    spi->bits_per_word = 8;
    spi->max_speed_hz = 10000000;

    ret = spi_setup(spi);
    if (ret)
        return ret;
    /* ... */
}
```

### Write with Page Boundary Handling

```c
static int demo_flash_write(struct demo_flash *flash, u32 addr,
                            const u8 *buf, size_t len)
{
    size_t page_offset, page_size, written = 0;

    while (written < len) {
        page_offset = (addr + written) % FLASH_PAGE_SIZE;
        page_size = min(FLASH_PAGE_SIZE - page_offset, len - written);

        ret = demo_flash_program_page(flash, addr + written,
                                      buf + written, page_size);
        if (ret)
            return ret;

        written += page_size;
    }
    return 0;
}
```

## Files

- `spi_flash_demo.c` - Main driver source
- `Makefile` - Build system
- `README.md` - This file

## Related Documentation

- Part 9.4: SPI Subsystem
- Part 9.5: SPI Driver
