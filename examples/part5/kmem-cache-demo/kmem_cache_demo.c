// SPDX-License-Identifier: GPL-2.0
/*
 * kmem_cache_demo.c - Slab allocator demonstration
 *
 * Demonstrates:
 * - Creating a custom kmem_cache
 * - Object constructor
 * - Allocating/freeing objects
 * - Cache statistics via /proc
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define CACHE_NAME "demo_objects"
#define MAX_OBJECTS 100

struct demo_object {
	struct list_head list;
	spinlock_t lock;
	int id;
	unsigned long created_at;
	char data[64];
	unsigned long access_count;
};

static struct kmem_cache *object_cache;
static LIST_HEAD(object_list);
static DEFINE_SPINLOCK(list_lock);
static atomic_t total_allocated = ATOMIC_INIT(0);
static atomic_t total_freed = ATOMIC_INIT(0);
static int next_id;

/* Constructor - called when slab page is allocated */
static void object_ctor(void *ptr)
{
	struct demo_object *obj = ptr;

	/* Initialize fields that stay constant across reuses */
	spin_lock_init(&obj->lock);
	INIT_LIST_HEAD(&obj->list);
	obj->access_count = 0;

	pr_debug("kmem_cache_demo: constructor called for %p\n", obj);
}

/* Allocate a new object */
static struct demo_object *alloc_object(const char *data)
{
	struct demo_object *obj;

	obj = kmem_cache_alloc(object_cache, GFP_KERNEL);
	if (!obj)
		return NULL;

	/* Initialize per-allocation fields */
	obj->id = next_id++;
	obj->created_at = jiffies;
	strscpy(obj->data, data, sizeof(obj->data));
	obj->access_count = 0;

	spin_lock(&list_lock);
	list_add(&obj->list, &object_list);
	spin_unlock(&list_lock);

	atomic_inc(&total_allocated);

	pr_info("kmem_cache_demo: allocated object %d\n", obj->id);
	return obj;
}

/* Free an object */
static void free_object(struct demo_object *obj)
{
	int id = obj->id;

	spin_lock(&list_lock);
	list_del(&obj->list);
	spin_unlock(&list_lock);

	kmem_cache_free(object_cache, obj);
	atomic_inc(&total_freed);

	pr_info("kmem_cache_demo: freed object %d\n", id);
}

/* Free all objects */
static void free_all_objects(void)
{
	struct demo_object *obj, *tmp;

	spin_lock(&list_lock);
	list_for_each_entry_safe(obj, tmp, &object_list, list) {
		list_del(&obj->list);
		kmem_cache_free(object_cache, obj);
		atomic_inc(&total_freed);
	}
	spin_unlock(&list_lock);
}

/* Access an object */
static void access_object(struct demo_object *obj)
{
	spin_lock(&obj->lock);
	obj->access_count++;
	spin_unlock(&obj->lock);
}

/* Proc file interface */
static int stats_show(struct seq_file *m, void *v)
{
	struct demo_object *obj;
	int count = 0;

	seq_printf(m, "Slab Cache Demo Statistics\n");
	seq_printf(m, "==========================\n\n");
	seq_printf(m, "Cache name: %s\n", CACHE_NAME);
	seq_printf(m, "Object size: %zu bytes\n", sizeof(struct demo_object));
	seq_printf(m, "Total allocated: %d\n", atomic_read(&total_allocated));
	seq_printf(m, "Total freed: %d\n", atomic_read(&total_freed));
	seq_printf(m, "Currently active: %d\n",
		   atomic_read(&total_allocated) - atomic_read(&total_freed));

	seq_printf(m, "\nActive Objects:\n");

	spin_lock(&list_lock);
	list_for_each_entry(obj, &object_list, list) {
		seq_printf(m, "  [%d] data='%s' age=%lu ms accesses=%lu\n",
			   obj->id, obj->data,
			   jiffies_to_msecs(jiffies - obj->created_at),
			   obj->access_count);
		count++;
	}
	spin_unlock(&list_lock);

	if (count == 0)
		seq_printf(m, "  (none)\n");

	seq_printf(m, "\nCommands:\n");
	seq_printf(m, "  alloc <data> - Allocate new object\n");
	seq_printf(m, "  free <id>    - Free object by ID\n");
	seq_printf(m, "  access <id>  - Increment access count\n");
	seq_printf(m, "  freeall      - Free all objects\n");

	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, stats_show, NULL);
}

static struct demo_object *find_object(int id)
{
	struct demo_object *obj;

	list_for_each_entry(obj, &object_list, list) {
		if (obj->id == id)
			return obj;
	}
	return NULL;
}

static ssize_t stats_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	char cmd[64];
	char data[32];
	int id;
	size_t len = min(count, sizeof(cmd) - 1);
	struct demo_object *obj;

	if (copy_from_user(cmd, buf, len))
		return -EFAULT;
	cmd[len] = '\0';

	if (cmd[len - 1] == '\n')
		cmd[len - 1] = '\0';

	if (sscanf(cmd, "alloc %31s", data) == 1) {
		obj = alloc_object(data);
		if (!obj)
			return -ENOMEM;
	} else if (sscanf(cmd, "free %d", &id) == 1) {
		spin_lock(&list_lock);
		obj = find_object(id);
		spin_unlock(&list_lock);
		if (obj)
			free_object(obj);
		else
			pr_warn("kmem_cache_demo: object %d not found\n", id);
	} else if (sscanf(cmd, "access %d", &id) == 1) {
		spin_lock(&list_lock);
		obj = find_object(id);
		if (obj)
			access_object(obj);
		spin_unlock(&list_lock);
	} else if (strncmp(cmd, "freeall", 7) == 0) {
		free_all_objects();
	} else {
		pr_warn("kmem_cache_demo: unknown command: %s\n", cmd);
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

static int __init kmem_cache_demo_init(void)
{
	pr_info("kmem_cache_demo: initializing\n");

	/* Create the slab cache */
	object_cache = kmem_cache_create(
		CACHE_NAME,			/* Cache name */
		sizeof(struct demo_object),	/* Object size */
		0,				/* Alignment (0 = natural) */
		SLAB_HWCACHE_ALIGN,		/* Flags */
		object_ctor			/* Constructor */
	);

	if (!object_cache) {
		pr_err("kmem_cache_demo: failed to create cache\n");
		return -ENOMEM;
	}

	/* Create proc entry */
	proc_entry = proc_create("kmem_cache_demo", 0666, NULL,
				 &stats_proc_ops);
	if (!proc_entry) {
		kmem_cache_destroy(object_cache);
		return -ENOMEM;
	}

	pr_info("kmem_cache_demo: cache created, object size=%zu\n",
		sizeof(struct demo_object));
	pr_info("kmem_cache_demo: use /proc/kmem_cache_demo to interact\n");

	return 0;
}

static void __exit kmem_cache_demo_exit(void)
{
	pr_info("kmem_cache_demo: exiting\n");

	/* Remove proc entry */
	proc_remove(proc_entry);

	/* Free all objects */
	free_all_objects();

	/* Destroy the cache */
	kmem_cache_destroy(object_cache);

	pr_info("kmem_cache_demo: exited\n");
}

module_init(kmem_cache_demo_init);
module_exit(kmem_cache_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Slab allocator demonstration");
MODULE_VERSION("1.0");
