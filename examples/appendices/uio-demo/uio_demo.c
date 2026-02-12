// SPDX-License-Identifier: GPL-2.0
/*
 * uio_demo.c - UIO (Userspace I/O) kernel stub
 *
 * Demonstrates:
 * - Minimal UIO kernel driver
 * - uio_info registration
 * - Memory region export to user space
 * - User space can mmap and access device memory
 *
 * This demo uses a kernel-allocated buffer as simulated
 * "device memory" that user space can read/write via mmap.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/slab.h>

#define UIO_DEMO_MEM_SIZE 4096

struct uio_demo_dev {
	struct uio_info info;
	void *mem;  /* Simulated device memory */
};

static int uio_demo_probe(struct platform_device *pdev)
{
	struct uio_demo_dev *dev;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* Allocate simulated device memory */
	dev->mem = devm_kzalloc(&pdev->dev, UIO_DEMO_MEM_SIZE, GFP_KERNEL);
	if (!dev->mem)
		return -ENOMEM;

	/* Pre-fill with identification data */
	memcpy(dev->mem, "UIO-DEMO", 8);

	/* Configure UIO device */
	dev->info.name = "uio-demo";
	dev->info.version = "1.0";
	dev->info.irq = UIO_IRQ_NONE;  /* No IRQ for this demo */

	/* Export memory region to user space */
	dev->info.mem[0].name = "device_memory";
	dev->info.mem[0].addr = (phys_addr_t)(uintptr_t)dev->mem;
	dev->info.mem[0].size = UIO_DEMO_MEM_SIZE;
	dev->info.mem[0].memtype = UIO_MEM_LOGICAL;

	platform_set_drvdata(pdev, dev);

	return devm_uio_register_device(&pdev->dev, &dev->info);
}

/* Platform device for self-registration (demo only) */
static struct platform_device *pdev;

static int __init uio_demo_init(void)
{
	int ret;

	pdev = platform_device_alloc("uio-demo", -1);
	if (!pdev)
		return -ENOMEM;

	ret = platform_device_add(pdev);
	if (ret) {
		platform_device_put(pdev);
		return ret;
	}

	pr_info("uio_demo: device registered as /dev/uioN\n");
	return 0;
}

static void __exit uio_demo_exit(void)
{
	platform_device_unregister(pdev);
	pr_info("uio_demo: unregistered\n");
}

static struct platform_driver uio_demo_driver = {
	.probe = uio_demo_probe,
	.driver = {
		.name = "uio-demo",
	},
};

module_platform_driver(uio_demo_driver);
module_init(uio_demo_init);
module_exit(uio_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("UIO demonstration kernel stub");
MODULE_VERSION("1.0");
