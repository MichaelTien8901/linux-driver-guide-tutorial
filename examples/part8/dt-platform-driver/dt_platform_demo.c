// SPDX-License-Identifier: GPL-2.0
/*
 * Device Tree Platform Driver Demo
 *
 * Demonstrates a platform driver that:
 * - Matches via Device Tree compatible string
 * - Reads various DT properties (integers, strings, booleans)
 * - Uses named resources (memory regions, clocks, GPIOs)
 * - Handles optional properties with defaults
 * - Iterates child nodes
 *
 * This is a virtual device for demonstration purposes.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/property.h>
#include <linux/gpio/consumer.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DRIVER_NAME "dt_platform_demo"
#define MAX_CHANNELS 8

/* Device variant data */
struct dt_demo_variant {
	const char *name;
	int max_channels;
	bool has_dma;
};

/* Per-channel configuration from DT */
struct dt_demo_channel {
	u32 reg;
	const char *label;
	const char *mode;
};

/* Per-device data */
struct dt_demo_device {
	struct device *dev;
	const struct dt_demo_variant *variant;

	/* Resources from DT */
	struct gpio_desc *reset_gpio;
	struct gpio_desc *enable_gpio;
	struct clk *clk;
	struct regulator *vdd;

	/* Properties from DT */
	u32 buffer_size;
	u32 timeout_ms;
	const char *mode;
	bool feature_enabled;

	/* Child node data */
	int num_channels;
	struct dt_demo_channel channels[MAX_CHANNELS];
};

static struct dt_demo_device *demo_dev;
static struct proc_dir_entry *proc_file;

/* Variant definitions */
static const struct dt_demo_variant variant_v1 = {
	.name = "dt-demo-v1",
	.max_channels = 4,
	.has_dma = false,
};

static const struct dt_demo_variant variant_v2 = {
	.name = "dt-demo-v2",
	.max_channels = 8,
	.has_dma = true,
};

/* Parse child nodes */
static int dt_demo_parse_channels(struct dt_demo_device *demo)
{
	struct device_node *np = demo->dev->of_node;
	struct device_node *child;
	int i = 0;

	for_each_child_of_node(np, child) {
		if (i >= demo->variant->max_channels) {
			dev_warn(demo->dev, "Too many channels, max %d\n",
				 demo->variant->max_channels);
			of_node_put(child);
			break;
		}

		if (of_property_read_u32(child, "reg", &demo->channels[i].reg)) {
			dev_err(demo->dev, "Channel missing 'reg' property\n");
			of_node_put(child);
			return -EINVAL;
		}

		/* Optional properties with defaults */
		if (of_property_read_string(child, "label",
					    &demo->channels[i].label))
			demo->channels[i].label = "unnamed";

		if (of_property_read_string(child, "demo,mode",
					    &demo->channels[i].mode))
			demo->channels[i].mode = "default";

		dev_info(demo->dev, "Channel %d: reg=%u, label=%s, mode=%s\n",
			 i, demo->channels[i].reg,
			 demo->channels[i].label,
			 demo->channels[i].mode);
		i++;
	}

	demo->num_channels = i;
	return 0;
}

/* Proc file to show configuration */
static int dt_demo_show(struct seq_file *m, void *v)
{
	struct dt_demo_device *demo = demo_dev;
	int i;

	if (!demo)
		return -ENODEV;

	seq_puts(m, "Device Tree Platform Driver Demo\n");
	seq_puts(m, "================================\n\n");

	seq_printf(m, "Variant:         %s\n", demo->variant->name);
	seq_printf(m, "Max channels:    %d\n", demo->variant->max_channels);
	seq_printf(m, "Has DMA:         %s\n", demo->variant->has_dma ? "yes" : "no");
	seq_puts(m, "\n");

	seq_puts(m, "Properties from Device Tree:\n");
	seq_printf(m, "  buffer-size:   %u bytes\n", demo->buffer_size);
	seq_printf(m, "  timeout-ms:    %u ms\n", demo->timeout_ms);
	seq_printf(m, "  mode:          %s\n", demo->mode);
	seq_printf(m, "  feature:       %s\n", demo->feature_enabled ? "enabled" : "disabled");
	seq_puts(m, "\n");

	seq_puts(m, "Resources:\n");
	seq_printf(m, "  Reset GPIO:    %s\n", demo->reset_gpio ? "present" : "not present");
	seq_printf(m, "  Enable GPIO:   %s\n", demo->enable_gpio ? "present" : "not present");
	seq_printf(m, "  Clock:         %s\n", demo->clk ? "present" : "not present");
	seq_printf(m, "  Regulator:     %s\n", demo->vdd ? "present" : "not present");
	seq_puts(m, "\n");

	seq_printf(m, "Channels: %d\n", demo->num_channels);
	for (i = 0; i < demo->num_channels; i++) {
		seq_printf(m, "  Channel %d:\n", i);
		seq_printf(m, "    reg:    %u\n", demo->channels[i].reg);
		seq_printf(m, "    label:  %s\n", demo->channels[i].label);
		seq_printf(m, "    mode:   %s\n", demo->channels[i].mode);
	}

	return 0;
}

static int dt_demo_open(struct inode *inode, struct file *file)
{
	return single_open(file, dt_demo_show, NULL);
}

static const struct proc_ops dt_demo_proc_ops = {
	.proc_open = dt_demo_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int dt_demo_probe(struct platform_device *pdev)
{
	struct dt_demo_device *demo;
	const struct dt_demo_variant *variant;
	int ret;

	dev_info(&pdev->dev, "Probing device\n");

	/* Get variant data from match */
	variant = of_device_get_match_data(&pdev->dev);
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

	/* Read properties using device_property API (portable) */

	/* Required property */
	ret = device_property_read_u32(&pdev->dev, "demo,buffer-size",
				       &demo->buffer_size);
	if (ret) {
		dev_err(&pdev->dev, "Missing required 'demo,buffer-size'\n");
		return ret;
	}

	/* Optional property with default */
	if (device_property_read_u32(&pdev->dev, "demo,timeout-ms",
				     &demo->timeout_ms))
		demo->timeout_ms = 1000;  /* Default 1 second */

	/* String property */
	if (device_property_read_string(&pdev->dev, "demo,mode", &demo->mode))
		demo->mode = "normal";

	/* Boolean property */
	demo->feature_enabled = device_property_read_bool(&pdev->dev,
							  "demo,feature-enable");

	dev_info(&pdev->dev, "Properties: buffer=%u, timeout=%u, mode=%s, feature=%s\n",
		 demo->buffer_size, demo->timeout_ms, demo->mode,
		 demo->feature_enabled ? "on" : "off");

	/* Get optional GPIO - reset */
	demo->reset_gpio = devm_gpiod_get_optional(&pdev->dev, "reset",
						   GPIOD_OUT_LOW);
	if (IS_ERR(demo->reset_gpio))
		return dev_err_probe(&pdev->dev, PTR_ERR(demo->reset_gpio),
				     "Failed to get reset GPIO\n");

	/* Get optional GPIO - enable */
	demo->enable_gpio = devm_gpiod_get_optional(&pdev->dev, "enable",
						    GPIOD_OUT_LOW);
	if (IS_ERR(demo->enable_gpio))
		return PTR_ERR(demo->enable_gpio);

	/* Get optional clock */
	demo->clk = devm_clk_get_optional(&pdev->dev, "demo");
	if (IS_ERR(demo->clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(demo->clk),
				     "Failed to get clock\n");

	/* Get optional regulator */
	demo->vdd = devm_regulator_get_optional(&pdev->dev, "vdd");
	if (IS_ERR(demo->vdd)) {
		if (PTR_ERR(demo->vdd) == -ENODEV)
			demo->vdd = NULL;
		else
			return PTR_ERR(demo->vdd);
	}

	/* Parse child nodes */
	ret = dt_demo_parse_channels(demo);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, demo);
	demo_dev = demo;

	/* Create proc entry */
	proc_file = proc_create("dt_platform_demo", 0444, NULL, &dt_demo_proc_ops);

	dev_info(&pdev->dev, "Device probed successfully\n");
	dev_info(&pdev->dev, "View configuration: cat /proc/dt_platform_demo\n");

	return 0;
}

static int dt_demo_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "Removing device\n");

	if (proc_file)
		remove_proc_entry("dt_platform_demo", NULL);

	demo_dev = NULL;

	return 0;
}

/* Device Tree matching */
static const struct of_device_id dt_demo_of_match[] = {
	{ .compatible = "demo,dt-platform-v1", .data = &variant_v1 },
	{ .compatible = "demo,dt-platform-v2", .data = &variant_v2 },
	{ }
};
MODULE_DEVICE_TABLE(of, dt_demo_of_match);

/* Platform driver */
static struct platform_driver dt_demo_driver = {
	.probe = dt_demo_probe,
	.remove = dt_demo_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = dt_demo_of_match,
	},
};
module_platform_driver(dt_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Device Tree Platform Driver Demonstration");
MODULE_VERSION("1.0");
