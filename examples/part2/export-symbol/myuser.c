// SPDX-License-Identifier: GPL-2.0
/*
 * myuser.c - Module that uses symbols from mylib
 *
 * This module depends on mylib.ko and uses its exported symbols.
 * mylib.ko must be loaded first.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

/* Declare external functions from mylib */
extern int mylib_increment(void);
extern int mylib_get_count(void);
extern void mylib_reset(void);  /* GPL-only */
extern int mylib_multiply(int a, int b);

static int iterations = 5;
module_param(iterations, int, 0644);
MODULE_PARM_DESC(iterations, "Number of increment iterations (default: 5)");

static int __init myuser_init(void)
{
	int i, result;

	pr_info("myuser: User module loaded\n");
	pr_info("myuser: Initial count: %d\n", mylib_get_count());

	/* Use the library functions */
	pr_info("myuser: Incrementing %d times...\n", iterations);
	for (i = 0; i < iterations; i++) {
		result = mylib_increment();
		pr_info("myuser:   After increment %d: count = %d\n", i + 1, result);
	}

	/* Test multiply function */
	result = mylib_multiply(6, 7);
	pr_info("myuser: 6 * 7 = %d\n", result);

	pr_info("myuser: Final count: %d\n", mylib_get_count());

	return 0;
}

static void __exit myuser_exit(void)
{
	pr_info("myuser: Count before exit: %d\n", mylib_get_count());

	/* Reset the counter (GPL-only function) */
	mylib_reset();

	pr_info("myuser: User module unloaded\n");
}

module_init(myuser_init);
module_exit(myuser_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Module demonstrating use of exported symbols");
MODULE_VERSION("1.0");

/* Specify soft dependency on mylib */
MODULE_SOFTDEP("pre: mylib");
