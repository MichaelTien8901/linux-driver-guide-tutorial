// SPDX-License-Identifier: GPL-2.0
/*
 * SPI Flash Demo Driver
 *
 * Demonstrates SPI device driver with basic flash-like operations.
 * Provides a simple character device interface for read/write/erase.
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/mutex.h>

/* SPI Flash commands (simulated) */
#define CMD_READ_ID         0x9F
#define CMD_READ_STATUS     0x05
#define CMD_WRITE_ENABLE    0x06
#define CMD_WRITE_DISABLE   0x04
#define CMD_READ_DATA       0x03
#define CMD_PAGE_PROGRAM    0x02
#define CMD_SECTOR_ERASE    0x20
#define CMD_CHIP_ERASE      0xC7

/* Status register bits */
#define STATUS_WIP          BIT(0)  /* Write in progress */
#define STATUS_WEL          BIT(1)  /* Write enable latch */

/* Flash parameters */
#define FLASH_PAGE_SIZE     256
#define FLASH_SECTOR_SIZE   4096
#define FLASH_SIZE          (64 * 1024)  /* 64KB simulated flash */

#define DEMO_FLASH_MAGIC    0xDE

struct demo_flash {
    struct spi_device *spi;
    struct miscdevice miscdev;
    struct mutex lock;
    u8 *buffer;             /* Internal buffer for simulated storage */
    size_t size;
    u8 manufacturer_id;
    u16 device_id;
};

/* Read flash status register */
static int demo_flash_read_status(struct demo_flash *flash, u8 *status)
{
    u8 cmd = CMD_READ_STATUS;

    return spi_write_then_read(flash->spi, &cmd, 1, status, 1);
}

/* Wait for write operation to complete */
static int demo_flash_wait_ready(struct demo_flash *flash, unsigned int timeout_ms)
{
    u8 status;
    int ret;
    unsigned int elapsed = 0;

    while (elapsed < timeout_ms) {
        ret = demo_flash_read_status(flash, &status);
        if (ret)
            return ret;

        if (!(status & STATUS_WIP))
            return 0;

        usleep_range(1000, 2000);
        elapsed++;
    }

    return -ETIMEDOUT;
}

/* Enable write operations */
static int demo_flash_write_enable(struct demo_flash *flash)
{
    u8 cmd = CMD_WRITE_ENABLE;

    return spi_write(flash->spi, &cmd, 1);
}

/* Read flash ID */
static int demo_flash_read_id(struct demo_flash *flash)
{
    u8 cmd = CMD_READ_ID;
    u8 id[3];
    int ret;

    ret = spi_write_then_read(flash->spi, &cmd, 1, id, 3);
    if (ret)
        return ret;

    flash->manufacturer_id = id[0];
    flash->device_id = (id[1] << 8) | id[2];

    dev_info(&flash->spi->dev, "Flash ID: manufacturer=0x%02x, device=0x%04x\n",
             flash->manufacturer_id, flash->device_id);

    return 0;
}

/* Read data from flash */
static int demo_flash_read(struct demo_flash *flash, u32 addr, u8 *buf, size_t len)
{
    struct spi_transfer t[2];
    struct spi_message m;
    u8 cmd[4];
    int ret;

    if (addr + len > flash->size)
        return -EINVAL;

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

    ret = spi_sync(flash->spi, &m);
    if (ret)
        return ret;

    /* For simulation: copy from internal buffer */
    memcpy(buf, flash->buffer + addr, len);

    return 0;
}

/* Program a page (up to 256 bytes) */
static int demo_flash_program_page(struct demo_flash *flash, u32 addr,
                                   const u8 *buf, size_t len)
{
    struct spi_transfer t[2];
    struct spi_message m;
    u8 cmd[4];
    int ret;

    if (len > FLASH_PAGE_SIZE)
        return -EINVAL;

    if (addr + len > flash->size)
        return -EINVAL;

    ret = demo_flash_write_enable(flash);
    if (ret)
        return ret;

    /* Command + 24-bit address */
    cmd[0] = CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    memset(t, 0, sizeof(t));

    t[0].tx_buf = cmd;
    t[0].len = 4;

    t[1].tx_buf = buf;
    t[1].len = len;

    spi_message_init(&m);
    spi_message_add_tail(&t[0], &m);
    spi_message_add_tail(&t[1], &m);

    ret = spi_sync(flash->spi, &m);
    if (ret)
        return ret;

    /* For simulation: copy to internal buffer */
    memcpy(flash->buffer + addr, buf, len);

    /* Wait for programming to complete */
    return demo_flash_wait_ready(flash, 10);
}

/* Write data (handles page boundaries) */
static int demo_flash_write(struct demo_flash *flash, u32 addr,
                            const u8 *buf, size_t len)
{
    size_t page_offset, page_size, written = 0;
    int ret;

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

/* Erase a sector */
static int demo_flash_erase_sector(struct demo_flash *flash, u32 addr)
{
    u8 cmd[4];
    int ret;

    /* Align to sector boundary */
    addr &= ~(FLASH_SECTOR_SIZE - 1);

    if (addr >= flash->size)
        return -EINVAL;

    ret = demo_flash_write_enable(flash);
    if (ret)
        return ret;

    cmd[0] = CMD_SECTOR_ERASE;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    ret = spi_write(flash->spi, cmd, 4);
    if (ret)
        return ret;

    /* For simulation: fill sector with 0xFF */
    memset(flash->buffer + addr, 0xFF, FLASH_SECTOR_SIZE);

    /* Wait for erase to complete */
    return demo_flash_wait_ready(flash, 500);
}

/* Character device operations */
static int demo_flash_open(struct inode *inode, struct file *file)
{
    struct demo_flash *flash = container_of(file->private_data,
                                            struct demo_flash, miscdev);
    file->private_data = flash;
    return 0;
}

static ssize_t demo_flash_fread(struct file *file, char __user *buf,
                                size_t count, loff_t *ppos)
{
    struct demo_flash *flash = file->private_data;
    u8 *kbuf;
    int ret;

    if (*ppos >= flash->size)
        return 0;

    if (*ppos + count > flash->size)
        count = flash->size - *ppos;

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    mutex_lock(&flash->lock);
    ret = demo_flash_read(flash, *ppos, kbuf, count);
    mutex_unlock(&flash->lock);

    if (ret) {
        kfree(kbuf);
        return ret;
    }

    if (copy_to_user(buf, kbuf, count)) {
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    *ppos += count;
    return count;
}

static ssize_t demo_flash_fwrite(struct file *file, const char __user *buf,
                                 size_t count, loff_t *ppos)
{
    struct demo_flash *flash = file->private_data;
    u8 *kbuf;
    int ret;

    if (*ppos >= flash->size)
        return -ENOSPC;

    if (*ppos + count > flash->size)
        count = flash->size - *ppos;

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, buf, count)) {
        kfree(kbuf);
        return -EFAULT;
    }

    mutex_lock(&flash->lock);
    ret = demo_flash_write(flash, *ppos, kbuf, count);
    mutex_unlock(&flash->lock);

    kfree(kbuf);

    if (ret)
        return ret;

    *ppos += count;
    return count;
}

static loff_t demo_flash_llseek(struct file *file, loff_t offset, int whence)
{
    struct demo_flash *flash = file->private_data;

    return fixed_size_llseek(file, offset, whence, flash->size);
}

/* ioctl for erase operations */
#define FLASH_IOC_MAGIC     'F'
#define FLASH_IOC_ERASE     _IOW(FLASH_IOC_MAGIC, 1, u32)
#define FLASH_IOC_GETSIZE   _IOR(FLASH_IOC_MAGIC, 2, u32)

static long demo_flash_ioctl(struct file *file, unsigned int cmd,
                             unsigned long arg)
{
    struct demo_flash *flash = file->private_data;
    u32 addr;
    int ret;

    switch (cmd) {
    case FLASH_IOC_ERASE:
        if (copy_from_user(&addr, (void __user *)arg, sizeof(addr)))
            return -EFAULT;

        mutex_lock(&flash->lock);
        ret = demo_flash_erase_sector(flash, addr);
        mutex_unlock(&flash->lock);
        return ret;

    case FLASH_IOC_GETSIZE:
        if (copy_to_user((void __user *)arg, &flash->size, sizeof(flash->size)))
            return -EFAULT;
        return 0;

    default:
        return -ENOTTY;
    }
}

static const struct file_operations demo_flash_fops = {
    .owner = THIS_MODULE,
    .open = demo_flash_open,
    .read = demo_flash_fread,
    .write = demo_flash_fwrite,
    .llseek = demo_flash_llseek,
    .unlocked_ioctl = demo_flash_ioctl,
};

static int demo_flash_probe(struct spi_device *spi)
{
    struct demo_flash *flash;
    int ret;

    /* Configure SPI */
    spi->mode = SPI_MODE_0;
    spi->bits_per_word = 8;
    if (!spi->max_speed_hz)
        spi->max_speed_hz = 10000000;  /* 10 MHz default */

    ret = spi_setup(spi);
    if (ret) {
        dev_err(&spi->dev, "SPI setup failed: %d\n", ret);
        return ret;
    }

    /* Allocate driver data */
    flash = devm_kzalloc(&spi->dev, sizeof(*flash), GFP_KERNEL);
    if (!flash)
        return -ENOMEM;

    flash->spi = spi;
    flash->size = FLASH_SIZE;
    mutex_init(&flash->lock);

    /* Allocate internal buffer for simulation */
    flash->buffer = devm_kzalloc(&spi->dev, flash->size, GFP_KERNEL);
    if (!flash->buffer)
        return -ENOMEM;

    /* Initialize buffer to erased state (0xFF) */
    memset(flash->buffer, 0xFF, flash->size);

    /* Read and verify flash ID */
    ret = demo_flash_read_id(flash);
    if (ret)
        dev_warn(&spi->dev, "Failed to read flash ID: %d (simulation mode)\n", ret);

    /* For simulation, set dummy ID */
    if (!flash->manufacturer_id) {
        flash->manufacturer_id = DEMO_FLASH_MAGIC;
        flash->device_id = 0x4014;  /* Simulated 64KB flash */
    }

    /* Register misc device */
    flash->miscdev.minor = MISC_DYNAMIC_MINOR;
    flash->miscdev.name = "demo_flash";
    flash->miscdev.fops = &demo_flash_fops;
    flash->miscdev.parent = &spi->dev;

    ret = misc_register(&flash->miscdev);
    if (ret) {
        dev_err(&spi->dev, "Failed to register misc device: %d\n", ret);
        return ret;
    }

    spi_set_drvdata(spi, flash);

    dev_info(&spi->dev, "Demo SPI flash registered: %zu bytes, %d Hz\n",
             flash->size, spi->max_speed_hz);

    return 0;
}

static void demo_flash_remove(struct spi_device *spi)
{
    struct demo_flash *flash = spi_get_drvdata(spi);

    misc_deregister(&flash->miscdev);
    dev_info(&spi->dev, "Demo SPI flash removed\n");
}

static const struct spi_device_id demo_flash_id[] = {
    { "demo-flash", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, demo_flash_id);

static const struct of_device_id demo_flash_of_match[] = {
    { .compatible = "demo,spi-flash" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_flash_of_match);

static struct spi_driver demo_flash_driver = {
    .driver = {
        .name = "demo-flash",
        .of_match_table = demo_flash_of_match,
    },
    .probe = demo_flash_probe,
    .remove = demo_flash_remove,
    .id_table = demo_flash_id,
};
module_spi_driver(demo_flash_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Demo SPI Flash Driver");
