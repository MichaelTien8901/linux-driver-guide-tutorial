// SPDX-License-Identifier: GPL-2.0
/*
 * Platform Driver Demo
 *
 * Demonstrates a complete platform driver implementation including:
 * - Device matching via compatible string and ID table
 * - Probe and remove lifecycle
 * - Managed resources (devm_*)
 * - Device attributes (sysfs)
 * - Device private data
 *
 * This is a virtual device - no actual hardware required.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>

#define DRIVER_NAME "platform_demo"

/* Device variant information */
struct platform_demo_variant {
	const char *name;
	unsigned int max_channels;
	bool has_dma;
};

/* Per-device private data */
struct platform_demo_device {
	struct device *dev;
	void __iomem *regs;
	struct clk *clk;
	const struct platform_demo_variant *variant;

	/* Simulated device state */
	int value;
	int mode;
	bool enabled;
	unsigned int channel_count;
};

/* Device variants */
static const struct platform_demo_variant variant_basic = {
	.name = "basic",
	.max_channels = 2,
	.has_dma = false,
};

static const struct platform_demo_variant variant_advanced = {
	.name = "advanced",
	.max_channels = 8,
	.has_dma = true,
};

/* ============================================================
 * Sysfs Attributes
 * ============================================================ */

static ssize_t value_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct platform_demo_device *pdev = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", pdev->value);
}

static ssize_t value_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct platform_demo_device *pdev = dev_get_drvdata(dev);
	int val;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	if (val < 0 || val > 1000)
		return -EINVAL;

	pdev->value = val;
	dev_dbg(dev, "Value set to %d\n", val);

	return count;
}
static DEVICE_ATTR_RW(value);

static ssize_t mode_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct platform_demo_device *pdev = dev_get_drvdata(dev);
	static const char * const modes[] = { "idle", "active", "sleep" };

	if (pdev->mode >= ARRAY_SIZE(modes))
		return -EINVAL;

	return sprintf(buf, "%s\n", modes[pdev->mode]);
}

static ssize_t mode_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct platform_demo_device *pdev = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "idle"))
		pdev->mode = 0;
	else if (sysfs_streq(buf, "active"))
		pdev->mode = 1;
	else if (sysfs_streq(buf, "sleep"))
		pdev->mode = 2;
	else
		return -EINVAL;

	dev_dbg(dev, "Mode set to %d\n", pdev->mode);
	return count;
}
static DEVICE_ATTR_RW(mode);

static ssize_t enabled_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct platform_demo_device *pdev = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", pdev->enabled);
}

static ssize_t enabled_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct platform_demo_device *pdev = dev_get_drvdata(dev);
	bool val;

	if (kstrtobool(buf, &val))
		return -EINVAL;

	pdev->enabled = val;
	dev_dbg(dev, "Enabled set to %d\n", val);

	return count;
}
static DEVICE_ATTR_RW(enabled);

/* Read-only attributes */
static ssize_t variant_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct platform_demo_device *pdev = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", pdev->variant->name);
}
static DEVICE_ATTR_RO(variant);

static ssize_t max_channels_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct platform_demo_device *pdev = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", pdev->variant->max_channels);
}
static DEVICE_ATTR_RO(max_channels);

static ssize_t has_dma_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct platform_demo_device *pdev = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", pdev->variant->has_dma);
}
static DEVICE_ATTR_RO(has_dma);

/* Attribute array */
static struct attribute *platform_demo_attrs[] = {
	&dev_attr_value.attr,
	&dev_attr_mode.attr,
	&dev_attr_enabled.attr,
	&dev_attr_variant.attr,
	&dev_attr_max_channels.attr,
	&dev_attr_has_dma.attr,
	NULL,
};
ATTRIBUTE_GROUPS(platform_demo);

/* ============================================================
 * Platform Driver Probe/Remove
 * ============================================================ */

static int platform_demo_probe(struct platform_device *pdev)
{
	struct platform_demo_device *demo;
	const struct platform_demo_variant *variant;
	struct resource *res;
	int ret;

	dev_info(&pdev->dev, "Probing device\n");

	/* Get variant data from match */
	variant = of_device_get_match_data(&pdev->dev);
	if (!variant) {
		/* Try platform ID table */
		const struct platform_device_id *id;

		id = platform_get_device_id(pdev);
		if (id)
			variant = (const struct platform_demo_variant *)id->driver_data;
	}

	if (!variant) {
		dev_err(&pdev->dev, "No variant data found\n");
		return -ENODEV;
	}

	/* Allocate device structure */
	demo = devm_kzalloc(&pdev->dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	demo->dev = &pdev->dev;
	demo->variant = variant;

	/* Get memory resource (optional for this demo) */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		demo->regs = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(demo->regs)) {
			/* For demo, we continue without regs */
			dev_dbg(&pdev->dev, "No memory resource, running in simulation mode\n");
			demo->regs = NULL;
		}
	}

	/* Get clock (optional for this demo) */
	demo->clk = devm_clk_get_optional(&pdev->dev, "main");
	if (IS_ERR(demo->clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(demo->clk),
				     "Failed to get clock\n");

	if (demo->clk) {
		ret = clk_prepare_enable(demo->clk);
		if (ret)
			return dev_err_probe(&pdev->dev, ret,
					     "Failed to enable clock\n");

		/* Register cleanup for clock */
		ret = devm_add_action_or_reset(&pdev->dev,
			(void (*)(void *))clk_disable_unprepare, demo->clk);
		if (ret)
			return ret;
	}

	/* Initialize device state */
	demo->value = 100;
	demo->mode = 0;  /* idle */
	demo->enabled = true;
	demo->channel_count = demo->variant->max_channels;

	/* Store driver data */
	platform_set_drvdata(pdev, demo);

	/* Enable runtime PM */
	pm_runtime_enable(&pdev->dev);

	dev_info(&pdev->dev, "Device probed: variant=%s, channels=%u, dma=%s\n",
		 variant->name, variant->max_channels,
		 variant->has_dma ? "yes" : "no");

	return 0;
}

static int platform_demo_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "Removing device\n");

	pm_runtime_disable(&pdev->dev);

	/* All devm_* resources are automatically released */

	return 0;
}

/* ============================================================
 * Power Management
 * ============================================================ */

static int __maybe_unused platform_demo_suspend(struct device *dev)
{
	struct platform_demo_device *demo = dev_get_drvdata(dev);

	dev_dbg(dev, "Suspending device (value=%d, mode=%d)\n",
		demo->value, demo->mode);

	/* Save state, disable hardware, etc. */

	return 0;
}

static int __maybe_unused platform_demo_resume(struct device *dev)
{
	struct platform_demo_device *demo = dev_get_drvdata(dev);

	dev_dbg(dev, "Resuming device\n");

	/* Restore state, enable hardware, etc. */
	/* Value and mode are preserved in demo struct */

	return 0;
}

static int __maybe_unused platform_demo_runtime_suspend(struct device *dev)
{
	struct platform_demo_device *demo = dev_get_drvdata(dev);

	dev_dbg(dev, "Runtime suspend\n");

	if (demo->clk)
		clk_disable(demo->clk);

	return 0;
}

static int __maybe_unused platform_demo_runtime_resume(struct device *dev)
{
	struct platform_demo_device *demo = dev_get_drvdata(dev);
	int ret = 0;

	dev_dbg(dev, "Runtime resume\n");

	if (demo->clk) {
		ret = clk_enable(demo->clk);
		if (ret)
			dev_err(dev, "Failed to enable clock: %d\n", ret);
	}

	return ret;
}

static const struct dev_pm_ops platform_demo_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(platform_demo_suspend, platform_demo_resume)
	SET_RUNTIME_PM_OPS(platform_demo_runtime_suspend,
			   platform_demo_runtime_resume, NULL)
};

/* ============================================================
 * Device Matching
 * ============================================================ */

/* Device Tree matching */
static const struct of_device_id platform_demo_of_match[] = {
	{ .compatible = "demo,platform-basic", .data = &variant_basic },
	{ .compatible = "demo,platform-advanced", .data = &variant_advanced },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, platform_demo_of_match);

/* Platform ID table (for non-DT matching) */
static const struct platform_device_id platform_demo_id_table[] = {
	{ "platform-demo-basic", (kernel_ulong_t)&variant_basic },
	{ "platform-demo-advanced", (kernel_ulong_t)&variant_advanced },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(platform, platform_demo_id_table);

/* ============================================================
 * Platform Driver Structure
 * ============================================================ */

static struct platform_driver platform_demo_driver = {
	.probe = platform_demo_probe,
	.remove = platform_demo_remove,
	.id_table = platform_demo_id_table,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = platform_demo_of_match,
		.pm = &platform_demo_pm_ops,
		.dev_groups = platform_demo_groups,
	},
};

/* Module registration macro */
module_platform_driver(platform_demo_driver);

MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Platform Driver Demonstration");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
