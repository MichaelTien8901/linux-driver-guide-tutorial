// SPDX-License-Identifier: GPL-2.0
/*
 * params.c - Demonstrate kernel module parameters
 *
 * This module shows how to accept parameters at load time and
 * expose them through sysfs for runtime modification.
 *
 * Usage:
 *   insmod params.ko
 *   insmod params.ko count=10 name="Alice" verbose=1
 *   cat /sys/module/params/parameters/count
 *   echo 5 > /sys/module/params/parameters/count
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

/* Integer parameter with default value */
static int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of times to print greeting (default: 1)");

/* String parameter */
static char *name = "World";
module_param(name, charp, 0644);
MODULE_PARM_DESC(name, "Name to greet (default: World)");

/* Boolean parameter */
static bool verbose = false;
module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Enable verbose output (default: false)");

/* Array parameter */
static int values[10];
static int values_count;
module_param_array(values, int, &values_count, 0644);
MODULE_PARM_DESC(values, "Array of integer values");

/* Parameter with callback for validation/notification */
static int validated_param = 50;

static int validated_param_set(const char *val, const struct kernel_param *kp)
{
	int new_val;
	int ret = kstrtoint(val, 10, &new_val);

	if (ret)
		return ret;

	/* Validate range */
	if (new_val < 0 || new_val > 100) {
		pr_err("params: validated_param must be 0-100, got %d\n", new_val);
		return -EINVAL;
	}

	pr_info("params: validated_param changed from %d to %d\n",
		validated_param, new_val);
	validated_param = new_val;
	return 0;
}

static int validated_param_get(char *buf, const struct kernel_param *kp)
{
	return sprintf(buf, "%d\n", validated_param);
}

static const struct kernel_param_ops validated_param_ops = {
	.set = validated_param_set,
	.get = validated_param_get,
};

module_param_cb(validated, &validated_param_ops, NULL, 0644);
MODULE_PARM_DESC(validated, "Parameter with validation (0-100, default: 50)");

static int __init params_init(void)
{
	int i;

	pr_info("params: Module loaded\n");
	pr_info("params: Parameters:\n");
	pr_info("params:   count = %d\n", count);
	pr_info("params:   name = %s\n", name);
	pr_info("params:   verbose = %s\n", verbose ? "true" : "false");
	pr_info("params:   validated = %d\n", validated_param);

	if (values_count > 0) {
		pr_info("params:   values (%d elements):", values_count);
		for (i = 0; i < values_count; i++)
			pr_cont(" %d", values[i]);
		pr_cont("\n");
	}

	/* Print greeting */
	for (i = 0; i < count; i++) {
		if (verbose)
			pr_info("params: [%d/%d] Hello, %s!\n", i + 1, count, name);
		else
			pr_info("params: Hello, %s!\n", name);
	}

	return 0;
}

static void __exit params_exit(void)
{
	pr_info("params: Final parameter values:\n");
	pr_info("params:   count = %d\n", count);
	pr_info("params:   name = %s\n", name);
	pr_info("params:   verbose = %s\n", verbose ? "true" : "false");
	pr_info("params:   validated = %d\n", validated_param);
	pr_info("params: Module unloaded\n");
}

module_init(params_init);
module_exit(params_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Demonstrate kernel module parameters");
MODULE_VERSION("1.0");
