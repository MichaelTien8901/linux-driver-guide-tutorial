// SPDX-License-Identifier: GPL-2.0
/*
 * Threaded IRQ Demonstration Module
 *
 * Demonstrates threaded interrupt handling patterns:
 * - Hardirq handler (fast acknowledgment)
 * - Threaded handler (can sleep, uses mutex)
 * - Data passing between handlers
 * - Wait queue for user space notification
 *
 * This is a virtual device using a timer to simulate interrupts.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ktime.h>
#include <linux/random.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DRIVER_NAME "threaded_irq_demo"
#define BUFFER_SIZE 64

struct irq_demo_device {
	struct platform_device *pdev;
	struct timer_list timer;

	/* Simulated hardware state */
	atomic_t pending_irq;
	u32 hw_data;

	/* Data buffer protected by mutex (threaded handler can use mutex) */
	struct mutex data_mutex;
	u32 data_buffer[BUFFER_SIZE];
	int data_head;
	int data_count;

	/* Wait queue for readers */
	wait_queue_head_t data_ready;

	/* Statistics */
	atomic_t hardirq_count;
	atomic_t thread_count;
	atomic_t spurious_count;
	u64 last_irq_time;
	u64 total_latency_ns;

	/* Control */
	bool running;
	unsigned int interval_ms;
};

static struct irq_demo_device *demo_dev;
static struct proc_dir_entry *proc_dir;

/*
 * Simulated hardware interrupt trigger
 * In real hardware, this would be triggered by the device
 */
static void trigger_simulated_irq(struct timer_list *t)
{
	struct irq_demo_device *dev = from_timer(dev, t, timer);

	if (!dev->running)
		return;

	/* Simulate hardware generating data and interrupt */
	dev->hw_data = get_random_u32() & 0xFFFF;
	atomic_set(&dev->pending_irq, 1);
	dev->last_irq_time = ktime_get_ns();

	/* In real driver, hardware would trigger IRQ here */
	/* For demo, we call the handler chain directly */
	/* This simulates: device asserts IRQ line -> CPU receives interrupt */

	/* Re-arm timer for next "interrupt" */
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->interval_ms));
}

/*
 * Hardirq handler (top half)
 *
 * Runs in interrupt context with interrupts disabled.
 * Must be fast - just acknowledge and schedule thread.
 */
static irqreturn_t demo_hardirq(int irq, void *dev_id)
{
	struct irq_demo_device *dev = dev_id;

	/* Check if this interrupt is ours */
	if (!atomic_read(&dev->pending_irq)) {
		atomic_inc(&dev->spurious_count);
		return IRQ_NONE;
	}

	/* Acknowledge the interrupt (clear pending flag) */
	atomic_set(&dev->pending_irq, 0);
	atomic_inc(&dev->hardirq_count);

	/* Wake the threaded handler for processing */
	return IRQ_WAKE_THREAD;
}

/*
 * Threaded handler (bottom half)
 *
 * Runs in process context - CAN SLEEP!
 * Can use mutexes, allocate memory with GFP_KERNEL, etc.
 */
static irqreturn_t demo_thread_handler(int irq, void *dev_id)
{
	struct irq_demo_device *dev = dev_id;
	u64 now = ktime_get_ns();
	u64 latency;

	atomic_inc(&dev->thread_count);

	/* Calculate interrupt latency */
	latency = now - dev->last_irq_time;
	dev->total_latency_ns += latency;

	/* Lock mutex - WE CAN DO THIS because we're in process context! */
	mutex_lock(&dev->data_mutex);

	/* Store data in circular buffer */
	dev->data_buffer[dev->data_head] = dev->hw_data;
	dev->data_head = (dev->data_head + 1) % BUFFER_SIZE;
	if (dev->data_count < BUFFER_SIZE)
		dev->data_count++;

	mutex_unlock(&dev->data_mutex);

	/* Wake up any readers waiting for data */
	wake_up_interruptible(&dev->data_ready);

	return IRQ_HANDLED;
}

/*
 * Proc file: show statistics
 */
static int stats_show(struct seq_file *m, void *v)
{
	struct irq_demo_device *dev = demo_dev;
	int hardirq_cnt, thread_cnt, spurious_cnt;
	u64 avg_latency = 0;

	if (!dev)
		return -ENODEV;

	hardirq_cnt = atomic_read(&dev->hardirq_count);
	thread_cnt = atomic_read(&dev->thread_count);
	spurious_cnt = atomic_read(&dev->spurious_count);

	if (thread_cnt > 0)
		avg_latency = dev->total_latency_ns / thread_cnt;

	seq_puts(m, "Threaded IRQ Demo Statistics\n");
	seq_puts(m, "============================\n\n");

	seq_printf(m, "Running:           %s\n", dev->running ? "yes" : "no");
	seq_printf(m, "Interval:          %u ms\n", dev->interval_ms);
	seq_printf(m, "Hardirq count:     %d\n", hardirq_cnt);
	seq_printf(m, "Thread count:      %d\n", thread_cnt);
	seq_printf(m, "Spurious count:    %d\n", spurious_cnt);
	seq_printf(m, "Avg latency:       %llu ns\n", avg_latency);
	seq_printf(m, "Data in buffer:    %d\n", dev->data_count);

	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, stats_show, NULL);
}

static const struct proc_ops stats_proc_ops = {
	.proc_open = stats_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/*
 * Proc file: read data buffer
 */
static int data_show(struct seq_file *m, void *v)
{
	struct irq_demo_device *dev = demo_dev;
	int i, idx, count;

	if (!dev)
		return -ENODEV;

	mutex_lock(&dev->data_mutex);

	count = dev->data_count;
	seq_printf(m, "Data buffer (%d entries):\n", count);

	for (i = 0; i < count; i++) {
		idx = (dev->data_head - count + i + BUFFER_SIZE) % BUFFER_SIZE;
		seq_printf(m, "  [%2d]: 0x%04x\n", i, dev->data_buffer[idx]);
	}

	mutex_unlock(&dev->data_mutex);

	return 0;
}

static int data_open(struct inode *inode, struct file *file)
{
	return single_open(file, data_show, NULL);
}

static const struct proc_ops data_proc_ops = {
	.proc_open = data_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/*
 * Proc file: control (start/stop)
 */
static ssize_t control_write(struct file *file, const char __user *buf,
			     size_t count, loff_t *ppos)
{
	struct irq_demo_device *dev = demo_dev;
	char cmd[32];
	size_t len;

	if (!dev)
		return -ENODEV;

	len = min(count, sizeof(cmd) - 1);
	if (copy_from_user(cmd, buf, len))
		return -EFAULT;
	cmd[len] = '\0';

	/* Remove trailing newline */
	if (len > 0 && cmd[len - 1] == '\n')
		cmd[len - 1] = '\0';

	if (strcmp(cmd, "start") == 0) {
		if (!dev->running) {
			dev->running = true;
			mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->interval_ms));
			pr_info("threaded_irq_demo: Started\n");
		}
	} else if (strcmp(cmd, "stop") == 0) {
		dev->running = false;
		del_timer_sync(&dev->timer);
		pr_info("threaded_irq_demo: Stopped\n");
	} else if (strcmp(cmd, "reset") == 0) {
		atomic_set(&dev->hardirq_count, 0);
		atomic_set(&dev->thread_count, 0);
		atomic_set(&dev->spurious_count, 0);
		dev->total_latency_ns = 0;
		mutex_lock(&dev->data_mutex);
		dev->data_count = 0;
		dev->data_head = 0;
		mutex_unlock(&dev->data_mutex);
		pr_info("threaded_irq_demo: Stats reset\n");
	} else if (strncmp(cmd, "interval ", 9) == 0) {
		unsigned int interval;
		if (kstrtouint(cmd + 9, 10, &interval) == 0 && interval > 0) {
			dev->interval_ms = interval;
			pr_info("threaded_irq_demo: Interval set to %u ms\n", interval);
		}
	} else {
		return -EINVAL;
	}

	return count;
}

static int control_show(struct seq_file *m, void *v)
{
	seq_puts(m, "Commands:\n");
	seq_puts(m, "  start          - Start generating interrupts\n");
	seq_puts(m, "  stop           - Stop generating interrupts\n");
	seq_puts(m, "  reset          - Reset statistics\n");
	seq_puts(m, "  interval <ms>  - Set interrupt interval\n");
	return 0;
}

static int control_open(struct inode *inode, struct file *file)
{
	return single_open(file, control_show, NULL);
}

static const struct proc_ops control_proc_ops = {
	.proc_open = control_open,
	.proc_read = seq_read,
	.proc_write = control_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int demo_probe(struct platform_device *pdev)
{
	struct irq_demo_device *dev;

	dev_info(&pdev->dev, "Probing threaded IRQ demo device\n");

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdev = pdev;
	mutex_init(&dev->data_mutex);
	init_waitqueue_head(&dev->data_ready);
	atomic_set(&dev->pending_irq, 0);
	atomic_set(&dev->hardirq_count, 0);
	atomic_set(&dev->thread_count, 0);
	atomic_set(&dev->spurious_count, 0);
	dev->interval_ms = 100;  /* Default 100ms interval */
	dev->running = false;

	/* Initialize timer for simulating interrupts */
	timer_setup(&dev->timer, trigger_simulated_irq, 0);

	/*
	 * NOTE: In a real driver, you would request an actual IRQ here:
	 *
	 * irq = platform_get_irq(pdev, 0);
	 * ret = devm_request_threaded_irq(&pdev->dev, irq,
	 *                                 demo_hardirq,
	 *                                 demo_thread_handler,
	 *                                 IRQF_ONESHOT,
	 *                                 "demo", dev);
	 *
	 * For this demo, we simulate the IRQ using a timer that
	 * directly invokes the handler chain.
	 */

	platform_set_drvdata(pdev, dev);
	demo_dev = dev;

	/* Create proc entries */
	proc_dir = proc_mkdir("threaded_irq_demo", NULL);
	if (proc_dir) {
		proc_create("stats", 0444, proc_dir, &stats_proc_ops);
		proc_create("data", 0444, proc_dir, &data_proc_ops);
		proc_create("control", 0644, proc_dir, &control_proc_ops);
	}

	dev_info(&pdev->dev, "Threaded IRQ demo ready\n");
	dev_info(&pdev->dev, "Control via /proc/threaded_irq_demo/control\n");

	return 0;
}

static int demo_remove(struct platform_device *pdev)
{
	struct irq_demo_device *dev = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "Removing threaded IRQ demo device\n");

	/* Stop the timer */
	dev->running = false;
	del_timer_sync(&dev->timer);

	/* Remove proc entries */
	if (proc_dir) {
		remove_proc_entry("stats", proc_dir);
		remove_proc_entry("data", proc_dir);
		remove_proc_entry("control", proc_dir);
		remove_proc_entry("threaded_irq_demo", NULL);
	}

	demo_dev = NULL;

	return 0;
}

static struct platform_driver demo_driver = {
	.probe = demo_probe,
	.remove = demo_remove,
	.driver = {
		.name = DRIVER_NAME,
	},
};

/* Platform device for self-registration */
static struct platform_device *demo_pdev;

static int __init demo_init(void)
{
	int ret;

	pr_info("Threaded IRQ Demo: Initializing\n");

	ret = platform_driver_register(&demo_driver);
	if (ret)
		return ret;

	/* Create platform device to trigger probe */
	demo_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
	if (IS_ERR(demo_pdev)) {
		platform_driver_unregister(&demo_driver);
		return PTR_ERR(demo_pdev);
	}

	return 0;
}

static void __exit demo_exit(void)
{
	pr_info("Threaded IRQ Demo: Exiting\n");

	platform_device_unregister(demo_pdev);
	platform_driver_unregister(&demo_driver);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Guide");
MODULE_DESCRIPTION("Threaded IRQ handling demonstration");
MODULE_VERSION("1.0");
