// SPDX-License-Identifier: GPL-2.0
/*
 * timer_demo.c - Kernel timer demonstration
 *
 * Demonstrates:
 * - timer_list for periodic polling
 * - hrtimer for precise timing
 * - Jiffies and time conversions
 * - Proper timer cleanup
 */

#include <linux/module.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>

/* Statistics */
static atomic_t timer_fire_count = ATOMIC_INIT(0);
static atomic_t hrtimer_fire_count = ATOMIC_INIT(0);

/* Standard timer */
static struct timer_list poll_timer;
static unsigned long poll_interval_ms = 1000;  /* 1 second default */
static bool timer_running;

/* High-resolution timer */
static struct hrtimer hr_timer;
static ktime_t hr_interval;
static bool hrtimer_running;

/* Standard timer callback (softirq context - cannot sleep) */
static void poll_timer_callback(struct timer_list *t)
{
	atomic_inc(&timer_fire_count);
	pr_debug("timer_demo: timer fired (count: %d)\n",
		 atomic_read(&timer_fire_count));

	/* Re-arm for periodic operation */
	if (timer_running)
		mod_timer(&poll_timer,
			  jiffies + msecs_to_jiffies(poll_interval_ms));
}

/* High-resolution timer callback */
static enum hrtimer_restart hrtimer_callback(struct hrtimer *timer)
{
	atomic_inc(&hrtimer_fire_count);
	pr_debug("timer_demo: hrtimer fired (count: %d)\n",
		 atomic_read(&hrtimer_fire_count));

	if (hrtimer_running) {
		hrtimer_forward_now(timer, hr_interval);
		return HRTIMER_RESTART;
	}

	return HRTIMER_NORESTART;
}

static void start_timer(void)
{
	if (timer_running)
		return;

	timer_running = true;
	mod_timer(&poll_timer,
		  jiffies + msecs_to_jiffies(poll_interval_ms));
	pr_info("timer_demo: standard timer started (%lums)\n",
		poll_interval_ms);
}

static void stop_timer(void)
{
	timer_running = false;
	del_timer_sync(&poll_timer);
	pr_info("timer_demo: standard timer stopped\n");
}

static void start_hrtimer(void)
{
	if (hrtimer_running)
		return;

	hrtimer_running = true;
	hr_interval = ktime_set(0, 500000000);  /* 500ms */
	hrtimer_start(&hr_timer, hr_interval, HRTIMER_MODE_REL);
	pr_info("timer_demo: hrtimer started (500ms)\n");
}

static void stop_hrtimer(void)
{
	hrtimer_running = false;
	hrtimer_cancel(&hr_timer);
	pr_info("timer_demo: hrtimer stopped\n");
}

/* Proc file interface */
static int stats_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Timer Demo Statistics\n");
	seq_printf(m, "=====================\n\n");
	seq_printf(m, "Standard timer:\n");
	seq_printf(m, "  Running:    %s\n", timer_running ? "yes" : "no");
	seq_printf(m, "  Interval:   %lu ms\n", poll_interval_ms);
	seq_printf(m, "  Fire count: %d\n", atomic_read(&timer_fire_count));
	seq_printf(m, "\nHigh-resolution timer:\n");
	seq_printf(m, "  Running:    %s\n", hrtimer_running ? "yes" : "no");
	seq_printf(m, "  Interval:   500 ms\n");
	seq_printf(m, "  Fire count: %d\n", atomic_read(&hrtimer_fire_count));
	seq_printf(m, "\nSystem info:\n");
	seq_printf(m, "  HZ:         %d\n", HZ);
	seq_printf(m, "  Jiffies:    %lu\n", jiffies);
	seq_printf(m, "\nCommands: start, stop, hstart, hstop\n");
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

	if (strcmp(cmd, "start") == 0)
		start_timer();
	else if (strcmp(cmd, "stop") == 0)
		stop_timer();
	else if (strcmp(cmd, "hstart") == 0)
		start_hrtimer();
	else if (strcmp(cmd, "hstop") == 0)
		stop_hrtimer();
	else
		pr_warn("timer_demo: unknown command: %s\n", cmd);

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

static int __init timer_demo_init(void)
{
	pr_info("timer_demo: initializing\n");

	/* Setup standard timer */
	timer_setup(&poll_timer, poll_timer_callback, 0);

	/* Setup high-resolution timer */
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = hrtimer_callback;

	/* Create proc entry */
	proc_entry = proc_create("timer_demo", 0666, NULL, &stats_proc_ops);
	if (!proc_entry) {
		pr_err("timer_demo: failed to create proc entry\n");
		return -ENOMEM;
	}

	pr_info("timer_demo: initialized - use /proc/timer_demo\n");
	return 0;
}

static void __exit timer_demo_exit(void)
{
	pr_info("timer_demo: exiting\n");

	stop_timer();
	stop_hrtimer();

	if (proc_entry)
		proc_remove(proc_entry);

	pr_info("timer_demo: exited\n");
}

module_init(timer_demo_init);
module_exit(timer_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Kernel timer demonstration");
MODULE_VERSION("1.0");
