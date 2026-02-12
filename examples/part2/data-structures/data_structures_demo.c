// SPDX-License-Identifier: GPL-2.0
/*
 * data_structures_demo.c - Kernel data structures demonstration
 *
 * Demonstrates:
 * - Linked lists (list_head)
 * - Hash tables (DEFINE_HASHTABLE)
 * - Safe iteration and deletion
 * - container_of / list_entry usage
 */

#include <linux/module.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/* --- Linked List --- */

struct client {
	int id;
	char name[32];
	struct list_head list;
};

static LIST_HEAD(client_list);
static int client_count;

/* --- Hash Table --- */

struct device_entry {
	int dev_id;
	char description[48];
	struct hlist_node node;
};

static DEFINE_HASHTABLE(device_table, 6);  /* 2^6 = 64 buckets */
static int device_count;

/* --- Linked list operations --- */

static void add_client(int id, const char *name)
{
	struct client *c;

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return;

	c->id = id;
	strscpy(c->name, name, sizeof(c->name));
	list_add_tail(&c->list, &client_list);
	client_count++;
}

static void remove_client(int id)
{
	struct client *c, *tmp;

	list_for_each_entry_safe(c, tmp, &client_list, list) {
		if (c->id == id) {
			list_del(&c->list);
			kfree(c);
			client_count--;
			return;
		}
	}
}

static void clear_clients(void)
{
	struct client *c, *tmp;

	list_for_each_entry_safe(c, tmp, &client_list, list) {
		list_del(&c->list);
		kfree(c);
	}
	client_count = 0;
}

/* --- Hash table operations --- */

static void add_device(int id, const char *desc)
{
	struct device_entry *e;

	e = kmalloc(sizeof(*e), GFP_KERNEL);
	if (!e)
		return;

	e->dev_id = id;
	strscpy(e->description, desc, sizeof(e->description));
	hash_add(device_table, &e->node, e->dev_id);
	device_count++;
}

static struct device_entry *find_device(int id)
{
	struct device_entry *e;

	hash_for_each_possible(device_table, e, node, id) {
		if (e->dev_id == id)
			return e;
	}
	return NULL;
}

static void clear_devices(void)
{
	struct device_entry *e;
	struct hlist_node *tmp;
	int bkt;

	hash_for_each_safe(device_table, bkt, tmp, e, node) {
		hash_del(&e->node);
		kfree(e);
	}
	device_count = 0;
}

/* --- Proc interface --- */

static int stats_show(struct seq_file *m, void *v)
{
	struct client *c;
	struct device_entry *e;
	int bkt;

	seq_printf(m, "Data Structures Demo\n");
	seq_printf(m, "====================\n\n");

	/* Show linked list contents */
	seq_printf(m, "Linked List (%d clients):\n", client_count);
	list_for_each_entry(c, &client_list, list) {
		seq_printf(m, "  [%d] %s\n", c->id, c->name);
	}

	/* Show hash table contents */
	seq_printf(m, "\nHash Table (%d devices):\n", device_count);
	hash_for_each(device_table, bkt, e, node) {
		seq_printf(m, "  [%d] %s (bucket %d)\n",
			   e->dev_id, e->description, bkt);
	}

	seq_printf(m, "\nCommands: populate, clear, find <id>\n");
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

	if (strcmp(cmd, "populate") == 0) {
		/* Add sample data */
		add_client(1, "uart0");
		add_client(2, "spi1");
		add_client(3, "i2c2");
		add_client(4, "gpio3");

		add_device(100, "temperature sensor");
		add_device(200, "accelerometer");
		add_device(300, "display controller");
		add_device(400, "audio codec");

		pr_info("data_structures_demo: populated sample data\n");
	} else if (strcmp(cmd, "clear") == 0) {
		clear_clients();
		clear_devices();
		pr_info("data_structures_demo: cleared all data\n");
	} else if (strncmp(cmd, "find ", 5) == 0) {
		int id;
		struct device_entry *e;

		if (kstrtoint(cmd + 5, 10, &id) == 0) {
			e = find_device(id);
			if (e)
				pr_info("data_structures_demo: found device %d: %s\n",
					e->dev_id, e->description);
			else
				pr_info("data_structures_demo: device %d not found\n",
					id);
		}
	} else {
		pr_warn("data_structures_demo: unknown command: %s\n", cmd);
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

static int __init data_structures_demo_init(void)
{
	pr_info("data_structures_demo: initializing\n");

	hash_init(device_table);

	proc_entry = proc_create("data_structures_demo", 0666, NULL,
				 &stats_proc_ops);
	if (!proc_entry)
		return -ENOMEM;

	pr_info("data_structures_demo: use /proc/data_structures_demo\n");
	return 0;
}

static void __exit data_structures_demo_exit(void)
{
	clear_clients();
	clear_devices();

	if (proc_entry)
		proc_remove(proc_entry);

	pr_info("data_structures_demo: exited\n");
}

module_init(data_structures_demo_init);
module_exit(data_structures_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Kernel data structures demonstration");
MODULE_VERSION("1.0");
