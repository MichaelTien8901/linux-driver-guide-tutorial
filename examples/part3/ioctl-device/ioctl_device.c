// SPDX-License-Identifier: GPL-2.0
/*
 * ioctl_device.c - Character device with IOCTL support
 *
 * Demonstrates:
 * - IOCTL command handling
 * - _IO, _IOR, _IOW, _IOWR macros
 * - Data transfer in IOCTL
 * - Command validation
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#include "ioctl_example.h"

#define DEVICE_NAME "ioctl_example"
#define CLASS_NAME  "ioctl"

struct ioctl_device {
	struct cdev cdev;
	struct mutex lock;

	/* Configuration */
	int speed;
	int mode;
	char name[32];
	int value;

	/* Statistics */
	unsigned long read_count;
	unsigned long write_count;
	unsigned long ioctl_count;
	int last_error;
};

static struct ioctl_device ioctl_dev;
static dev_t dev_num;
static struct class *ioctl_class;
static struct device *ioctl_device;

static int ioctl_open(struct inode *inode, struct file *file)
{
	struct ioctl_device *dev;

	dev = container_of(inode->i_cdev, struct ioctl_device, cdev);
	file->private_data = dev;

	pr_info("ioctl_example: device opened\n");
	return 0;
}

static int ioctl_release(struct inode *inode, struct file *file)
{
	pr_info("ioctl_example: device closed\n");
	return 0;
}

static ssize_t ioctl_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	struct ioctl_device *dev = file->private_data;

	mutex_lock(&dev->lock);
	dev->read_count++;
	mutex_unlock(&dev->lock);

	/* Simple implementation - return device name */
	if (*ppos > 0)
		return 0;

	if (count > sizeof(dev->name))
		count = sizeof(dev->name);

	if (copy_to_user(buf, dev->name, count))
		return -EFAULT;

	*ppos = count;
	return count;
}

static ssize_t ioctl_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct ioctl_device *dev = file->private_data;

	mutex_lock(&dev->lock);
	dev->write_count++;

	if (count > sizeof(dev->name) - 1)
		count = sizeof(dev->name) - 1;

	if (copy_from_user(dev->name, buf, count)) {
		dev->last_error = -EFAULT;
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}

	dev->name[count] = '\0';
	mutex_unlock(&dev->lock);

	return count;
}

static long ioctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ioctl_device *dev = file->private_data;
	struct ioctl_config config;
	struct ioctl_stats stats;
	int value;
	int ret = 0;

	/* Verify magic number */
	if (_IOC_TYPE(cmd) != IOCTL_MAGIC) {
		pr_warn("ioctl_example: invalid magic number\n");
		return -ENOTTY;
	}

	/* Verify command number */
	if (_IOC_NR(cmd) > IOCTL_MAXNR) {
		pr_warn("ioctl_example: invalid command number\n");
		return -ENOTTY;
	}

	mutex_lock(&dev->lock);
	dev->ioctl_count++;

	switch (cmd) {
	case IOCTL_RESET:
		pr_info("ioctl_example: RESET command\n");
		dev->speed = 0;
		dev->mode = 0;
		dev->value = 0;
		memset(dev->name, 0, sizeof(dev->name));
		dev->read_count = 0;
		dev->write_count = 0;
		dev->ioctl_count = 0;
		dev->last_error = 0;
		break;

	case IOCTL_GET_STATS:
		pr_info("ioctl_example: GET_STATS command\n");
		stats.reads = dev->read_count;
		stats.writes = dev->write_count;
		stats.ioctls = dev->ioctl_count;
		stats.last_error = dev->last_error;

		if (copy_to_user((void __user *)arg, &stats, sizeof(stats))) {
			dev->last_error = -EFAULT;
			ret = -EFAULT;
		}
		break;

	case IOCTL_GET_VALUE:
		pr_info("ioctl_example: GET_VALUE command\n");
		if (put_user(dev->value, (int __user *)arg)) {
			dev->last_error = -EFAULT;
			ret = -EFAULT;
		}
		break;

	case IOCTL_SET_CONFIG:
		pr_info("ioctl_example: SET_CONFIG command\n");
		if (copy_from_user(&config, (void __user *)arg, sizeof(config))) {
			dev->last_error = -EFAULT;
			ret = -EFAULT;
			break;
		}

		/* Validate configuration */
		if (config.speed < 0 || config.speed > 1000) {
			pr_warn("ioctl_example: invalid speed %d\n", config.speed);
			dev->last_error = -EINVAL;
			ret = -EINVAL;
			break;
		}

		if (config.mode < 0 || config.mode > 3) {
			pr_warn("ioctl_example: invalid mode %d\n", config.mode);
			dev->last_error = -EINVAL;
			ret = -EINVAL;
			break;
		}

		dev->speed = config.speed;
		dev->mode = config.mode;
		strscpy(dev->name, config.name, sizeof(dev->name));

		pr_info("ioctl_example: config set: speed=%d, mode=%d, name=%s\n",
			dev->speed, dev->mode, dev->name);
		break;

	case IOCTL_SET_VALUE:
		pr_info("ioctl_example: SET_VALUE command\n");
		if (get_user(value, (int __user *)arg)) {
			dev->last_error = -EFAULT;
			ret = -EFAULT;
			break;
		}
		dev->value = value;
		break;

	case IOCTL_XFER_CONFIG:
		pr_info("ioctl_example: XFER_CONFIG command\n");

		/* Read input */
		if (copy_from_user(&config, (void __user *)arg, sizeof(config))) {
			dev->last_error = -EFAULT;
			ret = -EFAULT;
			break;
		}

		/* Process - store input, return current config */
		config.speed = dev->speed;
		config.mode = dev->mode;
		strscpy(config.name, dev->name, sizeof(config.name));

		/* Write output */
		if (copy_to_user((void __user *)arg, &config, sizeof(config))) {
			dev->last_error = -EFAULT;
			ret = -EFAULT;
		}
		break;

	default:
		pr_warn("ioctl_example: unknown command 0x%x\n", cmd);
		dev->last_error = -ENOTTY;
		ret = -ENOTTY;
	}

	mutex_unlock(&dev->lock);
	return ret;
}

static const struct file_operations ioctl_fops = {
	.owner          = THIS_MODULE,
	.open           = ioctl_open,
	.release        = ioctl_release,
	.read           = ioctl_read,
	.write          = ioctl_write,
	.unlocked_ioctl = ioctl_ioctl,
};

static int __init ioctl_init(void)
{
	int ret;

	/* Initialize device structure */
	mutex_init(&ioctl_dev.lock);
	strscpy(ioctl_dev.name, "default", sizeof(ioctl_dev.name));

	/* Allocate device numbers */
	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		pr_err("ioctl_example: failed to allocate device numbers\n");
		return ret;
	}

	/* Initialize cdev */
	cdev_init(&ioctl_dev.cdev, &ioctl_fops);
	ioctl_dev.cdev.owner = THIS_MODULE;

	/* Add cdev to system */
	ret = cdev_add(&ioctl_dev.cdev, dev_num, 1);
	if (ret < 0) {
		pr_err("ioctl_example: failed to add cdev\n");
		goto err_cdev;
	}

	/* Create device class */
	ioctl_class = class_create(CLASS_NAME);
	if (IS_ERR(ioctl_class)) {
		ret = PTR_ERR(ioctl_class);
		pr_err("ioctl_example: failed to create class\n");
		goto err_class;
	}

	/* Create device node */
	ioctl_device = device_create(ioctl_class, NULL, dev_num,
				     NULL, DEVICE_NAME);
	if (IS_ERR(ioctl_device)) {
		ret = PTR_ERR(ioctl_device);
		pr_err("ioctl_example: failed to create device\n");
		goto err_device;
	}

	pr_info("ioctl_example: registered with major=%d, minor=%d\n",
		MAJOR(dev_num), MINOR(dev_num));

	return 0;

err_device:
	class_destroy(ioctl_class);
err_class:
	cdev_del(&ioctl_dev.cdev);
err_cdev:
	unregister_chrdev_region(dev_num, 1);
	return ret;
}

static void __exit ioctl_exit(void)
{
	device_destroy(ioctl_class, dev_num);
	class_destroy(ioctl_class);
	cdev_del(&ioctl_dev.cdev);
	unregister_chrdev_region(dev_num, 1);

	pr_info("ioctl_example: unregistered\n");
}

module_init(ioctl_init);
module_exit(ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("IOCTL example character device");
MODULE_VERSION("1.0");
