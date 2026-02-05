// SPDX-License-Identifier: GPL-2.0
/*
 * hello.c - A simple "Hello World" kernel module
 *
 * This module demonstrates the basic structure of a Linux kernel module:
 * - Module initialization function
 * - Module cleanup function
 * - Module metadata (license, author, description)
 *
 * Usage:
 *   Build:  make KERNEL_DIR=/path/to/kernel
 *   Load:   sudo insmod hello.ko
 *   Check:  dmesg | tail
 *   Unload: sudo rmmod hello
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

/*
 * Module initialization function
 *
 * Called when the module is loaded into the kernel.
 * The __init macro marks this function for removal after init,
 * saving memory since it's only needed once.
 *
 * Returns: 0 on success, negative errno on failure
 */
static int __init hello_init(void)
{
	pr_info("hello: Hello, World! Module loaded.\n");
	pr_info("hello: This is running in kernel space!\n");
	pr_info("hello: Current process: %s (pid %d)\n",
		current->comm, current->pid);

	return 0;
}

/*
 * Module cleanup function
 *
 * Called when the module is removed from the kernel.
 * The __exit macro marks this function for exclusion when
 * the module is built-in (not loadable).
 */
static void __exit hello_exit(void)
{
	pr_info("hello: Goodbye, World! Module unloaded.\n");
}

/* Register init and exit functions with the kernel */
module_init(hello_init);
module_exit(hello_exit);

/* Module metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("A simple Hello World kernel module");
MODULE_VERSION("1.0");
