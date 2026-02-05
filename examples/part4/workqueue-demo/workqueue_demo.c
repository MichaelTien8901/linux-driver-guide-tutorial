// SPDX-License-Identifier: GPL-2.0
/*
 * workqueue_demo.c - Work queue demonstration
 *
 * Demonstrates:
 * - System work queue (schedule_work)
 * - Delayed work
 * - Custom work queue
 * - Periodic work
 */

#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/delay.h>

/* Statistics */
static atomic_t immediate_work_count = ATOMIC_INIT(0);
static atomic_t delayed_work_count = ATOMIC_INIT(0);
static atomic_t custom_work_count = ATOMIC_INIT(0);
static atomic_t periodic_work_count = ATOMIC_INIT(0);

/* Custom work queue */
static struct workqueue_struct *custom_wq;

/* Work structures */
static struct work_struct immediate_work;
static struct delayed_work delayed_work;
static struct delayed_work periodic_work;
static bool periodic_running;

/* Custom work with data */
struct custom_work {
	struct work_struct work;
	int id;
	char message[64];
};

/* Immediate work handler */
static void immediate_work_handler(struct work_struct *work)
{
	atomic_inc(&immediate_work_count);
	pr_info("workqueue_demo: immediate work executing (count: %d)\n",
		atomic_read(&immediate_work_count));

	/* Can sleep in work queue context! */
	msleep(10);

	pr_info("workqueue_demo: immediate work done\n");
}

/* Delayed work handler */
static void delayed_work_handler(struct work_struct *work)
{
	atomic_inc(&delayed_work_count);
	pr_info("workqueue_demo: delayed work executing (count: %d)\n",
		atomic_read(&delayed_work_count));

	msleep(5);

	pr_info("workqueue_demo: delayed work done\n");
}

/* Custom work handler */
static void custom_work_handler(struct work_struct *work)
{
	struct custom_work *cw = container_of(work, struct custom_work, work);

	atomic_inc(&custom_work_count);
	pr_info("workqueue_demo: custom work id=%d msg='%s' (count: %d)\n",
		cw->id, cw->message, atomic_read(&custom_work_count));

	msleep(20);

	/* Free the work structure */
	kfree(cw);

	pr_info("workqueue_demo: custom work done\n");
}

/* Periodic work handler */
static void periodic_work_handler(struct work_struct *work)
{
	if (!periodic_running)
		return;

	atomic_inc(&periodic_work_count);
	pr_debug("workqueue_demo: periodic work (count: %d)\n",
		 atomic_read(&periodic_work_count));

	/* Reschedule ourselves */
	if (periodic_running)
		schedule_delayed_work(&periodic_work, HZ);  /* Every second */
}

/* Submit immediate work */
static void submit_immediate(void)
{
	schedule_work(&immediate_work);
}

/* Submit delayed work (2 seconds) */
static void submit_delayed(void)
{
	schedule_delayed_work(&delayed_work, 2 * HZ);
}

/* Submit custom work to dedicated queue */
static int submit_custom(int id, const char *msg)
{
	struct custom_work *cw;

	cw = kmalloc(sizeof(*cw), GFP_KERNEL);
	if (!cw)
		return -ENOMEM;

	cw->id = id;
	strscpy(cw->message, msg, sizeof(cw->message));
	INIT_WORK(&cw->work, custom_work_handler);

	queue_work(custom_wq, &cw->work);
	return 0;
}

/* Start periodic work */
static void start_periodic(void)
{
	if (periodic_running)
		return;

	periodic_running = true;
	schedule_delayed_work(&periodic_work, HZ);
	pr_info("workqueue_demo: periodic work started\n");
}

/* Stop periodic work */
static void stop_periodic(void)
{
	periodic_running = false;
	cancel_delayed_work_sync(&periodic_work);
	pr_info("workqueue_demo: periodic work stopped\n");
}

/* Proc file interface */
static int stats_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Work Queue Demo Statistics\n");
	seq_printf(m, "==========================\n\n");
	seq_printf(m, "Immediate work count: %d\n",
		   atomic_read(&immediate_work_count));
	seq_printf(m, "Delayed work count:   %d\n",
		   atomic_read(&delayed_work_count));
	seq_printf(m, "Custom work count:    %d\n",
		   atomic_read(&custom_work_count));
	seq_printf(m, "Periodic work count:  %d\n",
		   atomic_read(&periodic_work_count));
	seq_printf(m, "Periodic running:     %s\n",
		   periodic_running ? "yes" : "no");
	seq_printf(m, "\nWrite commands:\n");
	seq_printf(m, "  immediate - Queue immediate work\n");
	seq_printf(m, "  delayed   - Queue delayed work (2s)\n");
	seq_printf(m, "  custom    - Queue custom work\n");
	seq_printf(m, "  start     - Start periodic work\n");
	seq_printf(m, "  stop      - Stop periodic work\n");
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

	/* Remove trailing newline */
	if (len > 0 && cmd[len - 1] == '\n')
		cmd[len - 1] = '\0';

	if (strcmp(cmd, "immediate") == 0) {
		submit_immediate();
		pr_info("workqueue_demo: queued immediate work\n");
	} else if (strcmp(cmd, "delayed") == 0) {
		submit_delayed();
		pr_info("workqueue_demo: queued delayed work (2s)\n");
	} else if (strcmp(cmd, "custom") == 0) {
		static int custom_id;
		char msg[32];
		snprintf(msg, sizeof(msg), "custom job %d", ++custom_id);
		if (submit_custom(custom_id, msg) == 0)
			pr_info("workqueue_demo: queued custom work %d\n",
				custom_id);
	} else if (strcmp(cmd, "start") == 0) {
		start_periodic();
	} else if (strcmp(cmd, "stop") == 0) {
		stop_periodic();
	} else {
		pr_warn("workqueue_demo: unknown command: %s\n", cmd);
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

static int __init workqueue_demo_init(void)
{
	pr_info("workqueue_demo: initializing\n");

	/* Create custom work queue */
	custom_wq = alloc_workqueue("demo_wq",
				    WQ_UNBOUND | WQ_MEM_RECLAIM,
				    4);  /* max 4 active workers */
	if (!custom_wq) {
		pr_err("workqueue_demo: failed to create workqueue\n");
		return -ENOMEM;
	}

	/* Initialize work structures */
	INIT_WORK(&immediate_work, immediate_work_handler);
	INIT_DELAYED_WORK(&delayed_work, delayed_work_handler);
	INIT_DELAYED_WORK(&periodic_work, periodic_work_handler);

	/* Create proc entry */
	proc_entry = proc_create("workqueue_demo", 0666, NULL, &stats_proc_ops);
	if (!proc_entry) {
		destroy_workqueue(custom_wq);
		pr_err("workqueue_demo: failed to create proc entry\n");
		return -ENOMEM;
	}

	pr_info("workqueue_demo: initialized\n");
	pr_info("workqueue_demo: use /proc/workqueue_demo to interact\n");
	return 0;
}

static void __exit workqueue_demo_exit(void)
{
	pr_info("workqueue_demo: exiting\n");

	/* Stop periodic work */
	stop_periodic();

	/* Cancel any pending work */
	cancel_work_sync(&immediate_work);
	cancel_delayed_work_sync(&delayed_work);

	/* Flush and destroy custom queue */
	flush_workqueue(custom_wq);
	destroy_workqueue(custom_wq);

	/* Remove proc entry */
	if (proc_entry)
		proc_remove(proc_entry);

	pr_info("workqueue_demo: exited\n");
}

module_init(workqueue_demo_init);
module_exit(workqueue_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Work queue demonstration");
MODULE_VERSION("1.0");
