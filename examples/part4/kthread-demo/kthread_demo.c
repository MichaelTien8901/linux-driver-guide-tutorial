// SPDX-License-Identifier: GPL-2.0
/*
 * kthread_demo.c - Kernel thread demonstration
 *
 * Demonstrates:
 * - kthread_run() and kthread_stop()
 * - kthread_should_stop() cooperative shutdown
 * - Wait queues for event-driven sleeping
 * - Periodic polling with timeout
 */

#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/* Thread data */
static struct task_struct *poll_thread;
static struct task_struct *event_thread;

/* Shared state */
static DECLARE_WAIT_QUEUE_HEAD(event_wq);
static atomic_t poll_count = ATOMIC_INIT(0);
static atomic_t event_count = ATOMIC_INIT(0);
static bool event_pending;
static int simulated_sensor = 25;  /* "temperature" */

/*
 * Periodic polling thread
 * Reads a simulated sensor every second
 */
static int poll_thread_fn(void *data)
{
	pr_info("kthread_demo: poll thread started\n");

	while (!kthread_should_stop()) {
		/* Simulate reading hardware */
		simulated_sensor = 20 + (atomic_read(&poll_count) % 15);
		atomic_inc(&poll_count);

		pr_debug("kthread_demo: poll %d, sensor=%d\n",
			 atomic_read(&poll_count), simulated_sensor);

		/* Sleep for 1 second, but wake on stop request */
		wait_event_interruptible_timeout(event_wq,
						 kthread_should_stop(),
						 HZ);
	}

	pr_info("kthread_demo: poll thread stopping\n");
	return 0;
}

/*
 * Event-driven thread
 * Sleeps until woken by an external event
 */
static int event_thread_fn(void *data)
{
	pr_info("kthread_demo: event thread started\n");

	while (!kthread_should_stop()) {
		/* Sleep until event or stop */
		wait_event_interruptible(event_wq,
					 event_pending ||
					 kthread_should_stop());

		if (kthread_should_stop())
			break;

		/* Handle event */
		event_pending = false;
		atomic_inc(&event_count);

		pr_info("kthread_demo: event %d handled\n",
			atomic_read(&event_count));

		/* Simulate processing time */
		msleep(10);
	}

	pr_info("kthread_demo: event thread stopping\n");
	return 0;
}

/* Trigger an event (simulates external stimulus) */
static void trigger_event(void)
{
	event_pending = true;
	wake_up_interruptible(&event_wq);
	pr_info("kthread_demo: event triggered\n");
}

/* Proc file interface */
static int stats_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Kthread Demo Statistics\n");
	seq_printf(m, "=======================\n\n");
	seq_printf(m, "Poll thread:\n");
	seq_printf(m, "  Poll count:      %d\n", atomic_read(&poll_count));
	seq_printf(m, "  Sensor reading:  %d\n", simulated_sensor);
	seq_printf(m, "\nEvent thread:\n");
	seq_printf(m, "  Events handled:  %d\n", atomic_read(&event_count));
	seq_printf(m, "  Event pending:   %s\n",
		   event_pending ? "yes" : "no");
	seq_printf(m, "\nCommands: event\n");
	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, stats_show, NULL);
}

static ssize_t stats_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	char cmd[32];
	size_t len = min(count, sizeof(cmd) - 1);

	if (copy_from_user(cmd, buf, len))
		return -EFAULT;
	cmd[len] = '\0';

	if (len > 0 && cmd[len - 1] == '\n')
		cmd[len - 1] = '\0';

	if (strcmp(cmd, "event") == 0)
		trigger_event();
	else
		pr_warn("kthread_demo: unknown command: %s\n", cmd);

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

static int __init kthread_demo_init(void)
{
	pr_info("kthread_demo: initializing\n");

	/* Create proc entry first */
	proc_entry = proc_create("kthread_demo", 0666, NULL, &stats_proc_ops);
	if (!proc_entry) {
		pr_err("kthread_demo: failed to create proc entry\n");
		return -ENOMEM;
	}

	/* Start polling thread */
	poll_thread = kthread_run(poll_thread_fn, NULL, "kdemo_poll");
	if (IS_ERR(poll_thread)) {
		proc_remove(proc_entry);
		pr_err("kthread_demo: failed to create poll thread\n");
		return PTR_ERR(poll_thread);
	}

	/* Start event-driven thread */
	event_thread = kthread_run(event_thread_fn, NULL, "kdemo_event");
	if (IS_ERR(event_thread)) {
		kthread_stop(poll_thread);
		proc_remove(proc_entry);
		pr_err("kthread_demo: failed to create event thread\n");
		return PTR_ERR(event_thread);
	}

	pr_info("kthread_demo: initialized - use /proc/kthread_demo\n");
	return 0;
}

static void __exit kthread_demo_exit(void)
{
	pr_info("kthread_demo: exiting\n");

	/* Stop threads (blocks until each thread exits) */
	kthread_stop(event_thread);
	kthread_stop(poll_thread);

	if (proc_entry)
		proc_remove(proc_entry);

	pr_info("kthread_demo: exited\n");
}

module_init(kthread_demo_init);
module_exit(kthread_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Kernel thread demonstration");
MODULE_VERSION("1.0");
