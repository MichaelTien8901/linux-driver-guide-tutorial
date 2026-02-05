// SPDX-License-Identifier: GPL-2.0
/*
 * mylib.c - Library module that exports symbols
 *
 * This module exports functions and variables that can be
 * used by other modules. Must be loaded before dependent modules.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

/* Exported variable */
static int mylib_counter;

/* Exported function: increment counter and return value */
int mylib_increment(void)
{
	return ++mylib_counter;
}
EXPORT_SYMBOL(mylib_increment);

/* Exported function: get current counter value */
int mylib_get_count(void)
{
	return mylib_counter;
}
EXPORT_SYMBOL(mylib_get_count);

/* GPL-only exported function: reset counter */
void mylib_reset(void)
{
	mylib_counter = 0;
	pr_info("mylib: Counter reset to 0\n");
}
EXPORT_SYMBOL_GPL(mylib_reset);

/* Exported function: multiply two numbers */
int mylib_multiply(int a, int b)
{
	return a * b;
}
EXPORT_SYMBOL(mylib_multiply);

static int __init mylib_init(void)
{
	mylib_counter = 0;
	pr_info("mylib: Library module loaded\n");
	pr_info("mylib: Exported symbols: mylib_increment, mylib_get_count, mylib_reset, mylib_multiply\n");
	return 0;
}

static void __exit mylib_exit(void)
{
	pr_info("mylib: Library module unloaded (final count: %d)\n", mylib_counter);
}

module_init(mylib_init);
module_exit(mylib_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Library module demonstrating symbol export");
MODULE_VERSION("1.0");
