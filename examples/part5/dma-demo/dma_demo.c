// SPDX-License-Identifier: GPL-2.0
/*
 * dma_demo.c - DMA mapping demonstration
 *
 * Demonstrates:
 * - Coherent DMA allocation (dma_alloc_coherent)
 * - Streaming DMA mapping (dma_map_single)
 * - DMA synchronization
 *
 * Note: This is a demonstration module. Without actual hardware,
 * DMA operations are simulated.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DMA_BUFFER_SIZE 4096
#define COHERENT_BUFFER_SIZE 1024

struct dma_demo_dev {
	struct device *dev;

	/* Coherent DMA buffer (for descriptor ring simulation) */
	void *coherent_buf;
	dma_addr_t coherent_dma;

	/* Streaming DMA buffer */
	void *streaming_buf;
	dma_addr_t streaming_dma;
	bool streaming_mapped;

	/* Statistics */
	unsigned long coherent_writes;
	unsigned long coherent_reads;
	unsigned long streaming_maps;
	unsigned long streaming_unmaps;
};

static struct dma_demo_dev *demo_dev;
static struct platform_device *pdev;

/* Simulate writing to coherent buffer (like writing DMA descriptors) */
static void write_coherent(struct dma_demo_dev *dev, const char *data, size_t len)
{
	if (len > COHERENT_BUFFER_SIZE)
		len = COHERENT_BUFFER_SIZE;

	/* Direct write - always coherent with device */
	memcpy(dev->coherent_buf, data, len);

	/* Memory barrier ensures writes are visible */
	wmb();

	dev->coherent_writes++;
	pr_info("dma_demo: wrote %zu bytes to coherent buffer at dma=%pad\n",
		len, &dev->coherent_dma);
}

/* Simulate reading from coherent buffer */
static void read_coherent(struct dma_demo_dev *dev, char *data, size_t len)
{
	if (len > COHERENT_BUFFER_SIZE)
		len = COHERENT_BUFFER_SIZE;

	/* Memory barrier before read */
	rmb();

	memcpy(data, dev->coherent_buf, len);
	dev->coherent_reads++;

	pr_info("dma_demo: read %zu bytes from coherent buffer\n", len);
}

/* Map streaming buffer for device read (DMA_TO_DEVICE) */
static int map_streaming_to_device(struct dma_demo_dev *dev,
				   const char *data, size_t len)
{
	if (dev->streaming_mapped) {
		pr_warn("dma_demo: streaming buffer already mapped\n");
		return -EBUSY;
	}

	if (len > DMA_BUFFER_SIZE)
		len = DMA_BUFFER_SIZE;

	/* Fill buffer before mapping */
	memcpy(dev->streaming_buf, data, len);

	/* Map for device read */
	dev->streaming_dma = dma_map_single(dev->dev, dev->streaming_buf,
					    DMA_BUFFER_SIZE, DMA_TO_DEVICE);

	if (dma_mapping_error(dev->dev, dev->streaming_dma)) {
		pr_err("dma_demo: failed to map streaming buffer\n");
		return -ENOMEM;
	}

	dev->streaming_mapped = true;
	dev->streaming_maps++;

	pr_info("dma_demo: mapped streaming buffer for TO_DEVICE at dma=%pad\n",
		&dev->streaming_dma);

	return 0;
}

/* Map streaming buffer for device write (DMA_FROM_DEVICE) */
static int map_streaming_from_device(struct dma_demo_dev *dev)
{
	if (dev->streaming_mapped) {
		pr_warn("dma_demo: streaming buffer already mapped\n");
		return -EBUSY;
	}

	dev->streaming_dma = dma_map_single(dev->dev, dev->streaming_buf,
					    DMA_BUFFER_SIZE, DMA_FROM_DEVICE);

	if (dma_mapping_error(dev->dev, dev->streaming_dma)) {
		pr_err("dma_demo: failed to map streaming buffer\n");
		return -ENOMEM;
	}

	dev->streaming_mapped = true;
	dev->streaming_maps++;

	pr_info("dma_demo: mapped streaming buffer for FROM_DEVICE at dma=%pad\n",
		&dev->streaming_dma);

	return 0;
}

/* Unmap streaming buffer */
static void unmap_streaming(struct dma_demo_dev *dev, enum dma_data_direction dir)
{
	if (!dev->streaming_mapped) {
		pr_warn("dma_demo: streaming buffer not mapped\n");
		return;
	}

	dma_unmap_single(dev->dev, dev->streaming_dma,
			 DMA_BUFFER_SIZE, dir);

	dev->streaming_mapped = false;
	dev->streaming_unmaps++;

	pr_info("dma_demo: unmapped streaming buffer\n");
}

/* Sync streaming buffer for CPU access */
static void sync_streaming_for_cpu(struct dma_demo_dev *dev,
				   enum dma_data_direction dir)
{
	if (!dev->streaming_mapped) {
		pr_warn("dma_demo: buffer not mapped\n");
		return;
	}

	dma_sync_single_for_cpu(dev->dev, dev->streaming_dma,
				DMA_BUFFER_SIZE, dir);

	pr_info("dma_demo: synced streaming buffer for CPU\n");
}

/* Sync streaming buffer for device access */
static void sync_streaming_for_device(struct dma_demo_dev *dev,
				      enum dma_data_direction dir)
{
	if (!dev->streaming_mapped) {
		pr_warn("dma_demo: buffer not mapped\n");
		return;
	}

	dma_sync_single_for_device(dev->dev, dev->streaming_dma,
				   DMA_BUFFER_SIZE, dir);

	pr_info("dma_demo: synced streaming buffer for device\n");
}

/* Proc file interface */
static int stats_show(struct seq_file *m, void *v)
{
	struct dma_demo_dev *dev = demo_dev;

	seq_printf(m, "DMA Demo Statistics\n");
	seq_printf(m, "===================\n\n");

	seq_printf(m, "Coherent Buffer:\n");
	seq_printf(m, "  CPU address:  %p\n", dev->coherent_buf);
	seq_printf(m, "  DMA address:  %pad\n", &dev->coherent_dma);
	seq_printf(m, "  Size:         %d bytes\n", COHERENT_BUFFER_SIZE);
	seq_printf(m, "  Writes:       %lu\n", dev->coherent_writes);
	seq_printf(m, "  Reads:        %lu\n", dev->coherent_reads);

	seq_printf(m, "\nStreaming Buffer:\n");
	seq_printf(m, "  CPU address:  %p\n", dev->streaming_buf);
	seq_printf(m, "  Size:         %d bytes\n", DMA_BUFFER_SIZE);
	seq_printf(m, "  Mapped:       %s\n",
		   dev->streaming_mapped ? "yes" : "no");
	if (dev->streaming_mapped)
		seq_printf(m, "  DMA address:  %pad\n", &dev->streaming_dma);
	seq_printf(m, "  Total maps:   %lu\n", dev->streaming_maps);
	seq_printf(m, "  Total unmaps: %lu\n", dev->streaming_unmaps);

	seq_printf(m, "\nCommands:\n");
	seq_printf(m, "  cwrite <data>  - Write to coherent buffer\n");
	seq_printf(m, "  cread          - Read from coherent buffer\n");
	seq_printf(m, "  smap_to <data> - Map streaming for TO_DEVICE\n");
	seq_printf(m, "  smap_from      - Map streaming for FROM_DEVICE\n");
	seq_printf(m, "  sunmap_to      - Unmap streaming (TO_DEVICE)\n");
	seq_printf(m, "  sunmap_from    - Unmap streaming (FROM_DEVICE)\n");

	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, stats_show, NULL);
}

static ssize_t stats_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct dma_demo_dev *dev = demo_dev;
	char cmd[128];
	char data[64];
	size_t len = min(count, sizeof(cmd) - 1);

	if (copy_from_user(cmd, buf, len))
		return -EFAULT;
	cmd[len] = '\0';

	if (cmd[len - 1] == '\n')
		cmd[len - 1] = '\0';

	if (sscanf(cmd, "cwrite %63s", data) == 1) {
		write_coherent(dev, data, strlen(data) + 1);
	} else if (strcmp(cmd, "cread") == 0) {
		read_coherent(dev, data, sizeof(data));
		pr_info("dma_demo: coherent data: %s\n", data);
	} else if (sscanf(cmd, "smap_to %63s", data) == 1) {
		map_streaming_to_device(dev, data, strlen(data) + 1);
	} else if (strcmp(cmd, "smap_from") == 0) {
		map_streaming_from_device(dev);
	} else if (strcmp(cmd, "sunmap_to") == 0) {
		unmap_streaming(dev, DMA_TO_DEVICE);
	} else if (strcmp(cmd, "sunmap_from") == 0) {
		unmap_streaming(dev, DMA_FROM_DEVICE);
	} else if (strcmp(cmd, "sync_cpu") == 0) {
		sync_streaming_for_cpu(dev, DMA_FROM_DEVICE);
	} else if (strcmp(cmd, "sync_dev") == 0) {
		sync_streaming_for_device(dev, DMA_TO_DEVICE);
	} else {
		pr_warn("dma_demo: unknown command: %s\n", cmd);
	}

	return count;
}

static const struct proc_ops stats_proc_ops = {
	.proc_open = stats_open,
	.proc_read = seq_read,
	.proc_write = stats_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static struct proc_dir_entry *proc_entry;

static int dma_demo_probe(struct platform_device *platform_dev)
{
	struct device *dev = &platform_dev->dev;
	int ret;

	pr_info("dma_demo: probing\n");

	demo_dev = devm_kzalloc(dev, sizeof(*demo_dev), GFP_KERNEL);
	if (!demo_dev)
		return -ENOMEM;

	demo_dev->dev = dev;

	/* Set DMA mask */
	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "Failed to set DMA mask\n");
		return ret;
	}

	/* Allocate coherent DMA buffer */
	demo_dev->coherent_buf = dma_alloc_coherent(dev, COHERENT_BUFFER_SIZE,
						    &demo_dev->coherent_dma,
						    GFP_KERNEL);
	if (!demo_dev->coherent_buf) {
		dev_err(dev, "Failed to allocate coherent buffer\n");
		return -ENOMEM;
	}

	/* Allocate regular buffer for streaming DMA */
	demo_dev->streaming_buf = devm_kmalloc(dev, DMA_BUFFER_SIZE, GFP_KERNEL);
	if (!demo_dev->streaming_buf) {
		dma_free_coherent(dev, COHERENT_BUFFER_SIZE,
				  demo_dev->coherent_buf, demo_dev->coherent_dma);
		return -ENOMEM;
	}

	/* Create proc entry */
	proc_entry = proc_create("dma_demo", 0666, NULL, &stats_proc_ops);
	if (!proc_entry) {
		dma_free_coherent(dev, COHERENT_BUFFER_SIZE,
				  demo_dev->coherent_buf, demo_dev->coherent_dma);
		return -ENOMEM;
	}

	platform_set_drvdata(platform_dev, demo_dev);

	dev_info(dev, "DMA demo initialized\n");
	dev_info(dev, "Coherent buffer at cpu=%p dma=%pad\n",
		 demo_dev->coherent_buf, &demo_dev->coherent_dma);

	return 0;
}

static int dma_demo_remove(struct platform_device *platform_dev)
{
	struct dma_demo_dev *dev = platform_get_drvdata(platform_dev);

	pr_info("dma_demo: removing\n");

	proc_remove(proc_entry);

	if (dev->streaming_mapped)
		dma_unmap_single(dev->dev, dev->streaming_dma,
				 DMA_BUFFER_SIZE, DMA_BIDIRECTIONAL);

	dma_free_coherent(dev->dev, COHERENT_BUFFER_SIZE,
			  dev->coherent_buf, dev->coherent_dma);

	return 0;
}

static struct platform_driver dma_demo_driver = {
	.probe = dma_demo_probe,
	.remove = dma_demo_remove,
	.driver = {
		.name = "dma_demo",
	},
};

static int __init dma_demo_init(void)
{
	int ret;

	/* Register driver */
	ret = platform_driver_register(&dma_demo_driver);
	if (ret)
		return ret;

	/* Create a dummy platform device */
	pdev = platform_device_register_simple("dma_demo", -1, NULL, 0);
	if (IS_ERR(pdev)) {
		platform_driver_unregister(&dma_demo_driver);
		return PTR_ERR(pdev);
	}

	pr_info("dma_demo: module loaded\n");
	return 0;
}

static void __exit dma_demo_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&dma_demo_driver);
	pr_info("dma_demo: module unloaded\n");
}

module_init(dma_demo_init);
module_exit(dma_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("DMA mapping demonstration");
MODULE_VERSION("1.0");
