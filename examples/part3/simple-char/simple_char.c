// SPDX-License-Identifier: GPL-2.0
/*
 * simple_char.c - A simple character device driver
 *
 * Demonstrates:
 * - Character device registration with cdev
 * - File operations (open, release, read, write)
 * - Data transfer with copy_to_user/copy_from_user
 * - Automatic device node creation
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "simple_char"
#define CLASS_NAME  "simple"
#define BUFFER_SIZE 4096

struct simple_device {
	struct cdev cdev;
	char buffer[BUFFER_SIZE];
	size_t size;
	struct mutex lock;
};

static struct simple_device simple_dev;
static dev_t dev_num;
static struct class *simple_class;
static struct device *simple_device;

static int schar_open(struct inode *inode, struct file *file)
{
	struct simple_device *dev;

	dev = container_of(inode->i_cdev, struct simple_device, cdev);
	file->private_data = dev;

	pr_info("simple_char: device opened by %s (pid %d)\n",
		current->comm, current->pid);

	return 0;
}

static int schar_release(struct inode *inode, struct file *file)
{
	pr_info("simple_char: device closed\n");
	return 0;
}

static ssize_t schar_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct simple_device *dev = file->private_data;
	size_t available;

	mutex_lock(&dev->lock);

	/* Check if we're past end of data */
	if (*ppos >= dev->size) {
		mutex_unlock(&dev->lock);
		return 0;  /* EOF */
	}

	/* Calculate available data */
	available = dev->size - *ppos;
	if (count > available)
		count = available;

	/* Copy to user space */
	if (copy_to_user(buf, dev->buffer + *ppos, count)) {
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}

	/* Update position */
	*ppos += count;

	mutex_unlock(&dev->lock);

	pr_info("simple_char: read %zu bytes\n", count);

	return count;
}

static ssize_t schar_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	struct simple_device *dev = file->private_data;
	size_t space;

	mutex_lock(&dev->lock);

	/* Check for space */
	if (*ppos >= BUFFER_SIZE) {
		mutex_unlock(&dev->lock);
		return -ENOSPC;
	}

	/* Calculate available space */
	space = BUFFER_SIZE - *ppos;
	if (count > space)
		count = space;

	/* Copy from user space */
	if (copy_from_user(dev->buffer + *ppos, buf, count)) {
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}

	/* Update position and size */
	*ppos += count;
	if (*ppos > dev->size)
		dev->size = *ppos;

	mutex_unlock(&dev->lock);

	pr_info("simple_char: wrote %zu bytes\n", count);

	return count;
}

static loff_t simple_llseek(struct file *file, loff_t offset, int whence)
{
	struct simple_device *dev = file->private_data;
	loff_t newpos;

	mutex_lock(&dev->lock);

	switch (whence) {
	case SEEK_SET:
		newpos = offset;
		break;
	case SEEK_CUR:
		newpos = file->f_pos + offset;
		break;
	case SEEK_END:
		newpos = dev->size + offset;
		break;
	default:
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}

	if (newpos < 0) {
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}

	file->f_pos = newpos;

	mutex_unlock(&dev->lock);

	return newpos;
}

static const struct file_operations simple_fops = {
	.owner   = THIS_MODULE,
	.open    = schar_open,
	.release = schar_release,
	.read    = schar_read,
	.write   = schar_write,
	.llseek  = simple_llseek,
};

static int __init simple_init(void)
{
	int ret;

	/* Initialize device structure */
	mutex_init(&simple_dev.lock);
	simple_dev.size = 0;

	/* Allocate device numbers */
	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		pr_err("simple_char: failed to allocate device numbers\n");
		return ret;
	}

	/* Initialize cdev */
	cdev_init(&simple_dev.cdev, &simple_fops);
	simple_dev.cdev.owner = THIS_MODULE;

	/* Add cdev to system */
	ret = cdev_add(&simple_dev.cdev, dev_num, 1);
	if (ret < 0) {
		pr_err("simple_char: failed to add cdev\n");
		goto err_cdev;
	}

	/* Create device class */
	simple_class = class_create(CLASS_NAME);
	if (IS_ERR(simple_class)) {
		ret = PTR_ERR(simple_class);
		pr_err("simple_char: failed to create class\n");
		goto err_class;
	}

	/* Create device node */
	simple_device = device_create(simple_class, NULL, dev_num,
				      NULL, DEVICE_NAME);
	if (IS_ERR(simple_device)) {
		ret = PTR_ERR(simple_device);
		pr_err("simple_char: failed to create device\n");
		goto err_device;
	}

	pr_info("simple_char: registered with major=%d, minor=%d\n",
		MAJOR(dev_num), MINOR(dev_num));

	return 0;

err_device:
	class_destroy(simple_class);
err_class:
	cdev_del(&simple_dev.cdev);
err_cdev:
	unregister_chrdev_region(dev_num, 1);
	return ret;
}

static void __exit simple_exit(void)
{
	device_destroy(simple_class, dev_num);
	class_destroy(simple_class);
	cdev_del(&simple_dev.cdev);
	unregister_chrdev_region(dev_num, 1);

	pr_info("simple_char: unregistered\n");
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Simple character device driver example");
MODULE_VERSION("1.0");
