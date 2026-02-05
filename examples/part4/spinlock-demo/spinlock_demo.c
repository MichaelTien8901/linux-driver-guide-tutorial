// SPDX-License-Identifier: GPL-2.0
/*
 * spinlock_demo.c - Spinlock usage demonstration
 *
 * Demonstrates:
 * - Basic spinlock usage
 * - spin_lock_irqsave for interrupt-safe locking
 * - Reader-writer spinlocks
 * - Per-CPU data as an alternative
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/rwlock.h>
#include <linux/percpu.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define NUM_THREADS 4

/* Basic spinlock protected data */
struct basic_counter {
	spinlock_t lock;
	unsigned long count;
};

/* RW spinlock protected data */
struct rw_data {
	rwlock_t lock;
	int value;
	char name[32];
	unsigned long reads;
	unsigned long writes;
};

/* Per-CPU counter (no lock needed for per-cpu access) */
DEFINE_PER_CPU(unsigned long, percpu_counter);

static struct basic_counter basic = {
	.lock = __SPIN_LOCK_UNLOCKED(basic.lock),
	.count = 0,
};

static struct rw_data rwdata = {
	.lock = __RW_LOCK_UNLOCKED(rwdata.lock),
	.value = 0,
	.name = "initial",
	.reads = 0,
	.writes = 0,
};

static struct task_struct *threads[NUM_THREADS];
static bool stop_threads;

/* Thread that increments basic counter */
static int counter_thread(void *data)
{
	int id = (int)(long)data;

	pr_info("spinlock_demo: thread %d started\n", id);

	while (!kthread_should_stop() && !stop_threads) {
		spin_lock(&basic.lock);
		basic.count++;
		spin_unlock(&basic.lock);

		/* Small delay to not hog CPU */
		usleep_range(100, 200);
	}

	pr_info("spinlock_demo: thread %d stopped\n", id);
	return 0;
}

/* Thread that reads rw_data */
static int reader_thread(void *data)
{
	int id = (int)(long)data;
	int local_value;
	char local_name[32];

	pr_info("spinlock_demo: reader %d started\n", id);

	while (!kthread_should_stop() && !stop_threads) {
		read_lock(&rwdata.lock);
		local_value = rwdata.value;
		strscpy(local_name, rwdata.name, sizeof(local_name));
		rwdata.reads++;  /* Technically should be atomic, demo only */
		read_unlock(&rwdata.lock);

		/* Simulate using the data */
		if (local_value % 1000 == 0)
			pr_debug("Reader %d: value=%d, name=%s\n",
				 id, local_value, local_name);

		usleep_range(50, 100);
	}

	pr_info("spinlock_demo: reader %d stopped\n", id);
	return 0;
}

/* Thread that writes rw_data */
static int writer_thread(void *data)
{
	int id = (int)(long)data;

	pr_info("spinlock_demo: writer %d started\n", id);

	while (!kthread_should_stop() && !stop_threads) {
		write_lock(&rwdata.lock);
		rwdata.value++;
		snprintf(rwdata.name, sizeof(rwdata.name), "update_%d", rwdata.value);
		rwdata.writes++;
		write_unlock(&rwdata.lock);

		/* Writers are less frequent */
		msleep(10);
	}

	pr_info("spinlock_demo: writer %d stopped\n", id);
	return 0;
}

/* Thread using per-CPU counter */
static int percpu_thread(void *data)
{
	int id = (int)(long)data;

	pr_info("spinlock_demo: percpu thread %d started\n", id);

	while (!kthread_should_stop() && !stop_threads) {
		/* No lock needed for per-CPU data! */
		preempt_disable();
		this_cpu_inc(percpu_counter);
		preempt_enable();

		usleep_range(100, 200);
	}

	pr_info("spinlock_demo: percpu thread %d stopped\n", id);
	return 0;
}

/* Proc file to show stats */
static int stats_show(struct seq_file *m, void *v)
{
	unsigned long total_percpu = 0;
	int cpu;

	spin_lock(&basic.lock);
	seq_printf(m, "Basic counter: %lu\n", basic.count);
	spin_unlock(&basic.lock);

	read_lock(&rwdata.lock);
	seq_printf(m, "RW data:\n");
	seq_printf(m, "  value: %d\n", rwdata.value);
	seq_printf(m, "  name: %s\n", rwdata.name);
	seq_printf(m, "  reads: %lu\n", rwdata.reads);
	seq_printf(m, "  writes: %lu\n", rwdata.writes);
	read_unlock(&rwdata.lock);

	seq_printf(m, "Per-CPU counters:\n");
	for_each_possible_cpu(cpu) {
		unsigned long val = per_cpu(percpu_counter, cpu);
		seq_printf(m, "  CPU%d: %lu\n", cpu, val);
		total_percpu += val;
	}
	seq_printf(m, "  Total: %lu\n", total_percpu);

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

static struct proc_dir_entry *proc_entry;

static int __init spinlock_demo_init(void)
{
	int i;

	pr_info("spinlock_demo: initializing\n");

	/* Create proc entry */
	proc_entry = proc_create("spinlock_demo", 0444, NULL, &stats_proc_ops);
	if (!proc_entry) {
		pr_err("spinlock_demo: failed to create proc entry\n");
		return -ENOMEM;
	}

	/* Create threads */
	stop_threads = false;

	/* Counter threads */
	threads[0] = kthread_run(counter_thread, (void *)0L, "counter0");
	threads[1] = kthread_run(counter_thread, (void *)1L, "counter1");

	/* Reader thread */
	threads[2] = kthread_run(reader_thread, (void *)2L, "reader2");

	/* Writer thread */
	threads[3] = kthread_run(writer_thread, (void *)3L, "writer3");

	for (i = 0; i < NUM_THREADS; i++) {
		if (IS_ERR(threads[i])) {
			pr_err("spinlock_demo: failed to create thread %d\n", i);
			threads[i] = NULL;
		}
	}

	pr_info("spinlock_demo: initialized, check /proc/spinlock_demo\n");
	return 0;
}

static void __exit spinlock_demo_exit(void)
{
	int i;

	pr_info("spinlock_demo: exiting\n");

	/* Stop threads */
	stop_threads = true;
	for (i = 0; i < NUM_THREADS; i++) {
		if (threads[i])
			kthread_stop(threads[i]);
	}

	/* Remove proc entry */
	if (proc_entry)
		proc_remove(proc_entry);

	/* Print final stats */
	pr_info("spinlock_demo: final basic count: %lu\n", basic.count);
	pr_info("spinlock_demo: final rw writes: %lu\n", rwdata.writes);

	pr_info("spinlock_demo: exited\n");
}

module_init(spinlock_demo_init);
module_exit(spinlock_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Spinlock usage demonstration");
MODULE_VERSION("1.0");
