// SPDX-License-Identifier: GPL-2.0
/*
 * input_demo.c - Input subsystem demonstration
 *
 * Demonstrates:
 * - input_allocate_device() and input_register_device()
 * - Event capability declaration (EV_KEY)
 * - input_report_key() and input_sync()
 * - Virtual button triggered via proc interface
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/proc_fs.h>

static struct input_dev *vbutton;
static atomic_t press_count = ATOMIC_INIT(0);
static atomic_t release_count = ATOMIC_INIT(0);

static ssize_t vbutton_write(struct file *file, const char __user *buf,
			     size_t count, loff_t *ppos)
{
	char cmd[16];
	size_t len = min(count, sizeof(cmd) - 1);

	if (copy_from_user(cmd, buf, len))
		return -EFAULT;
	cmd[len] = '\0';

	if (len > 0 && cmd[len - 1] == '\n')
		cmd[len - 1] = '\0';

	if (strcmp(cmd, "press") == 0) {
		input_report_key(vbutton, KEY_ENTER, 1);
		input_sync(vbutton);
		atomic_inc(&press_count);
		pr_info("input_demo: key pressed\n");
	} else if (strcmp(cmd, "release") == 0) {
		input_report_key(vbutton, KEY_ENTER, 0);
		input_sync(vbutton);
		atomic_inc(&release_count);
		pr_info("input_demo: key released\n");
	} else if (strcmp(cmd, "click") == 0) {
		input_report_key(vbutton, KEY_ENTER, 1);
		input_sync(vbutton);
		input_report_key(vbutton, KEY_ENTER, 0);
		input_sync(vbutton);
		atomic_inc(&press_count);
		atomic_inc(&release_count);
		pr_info("input_demo: key clicked\n");
	} else if (strcmp(cmd, "power") == 0) {
		input_report_key(vbutton, KEY_POWER, 1);
		input_sync(vbutton);
		input_report_key(vbutton, KEY_POWER, 0);
		input_sync(vbutton);
		pr_info("input_demo: power key clicked\n");
	} else {
		pr_warn("input_demo: unknown command: %s\n", cmd);
		pr_warn("input_demo: use: press, release, click, power\n");
	}

	return count;
}

static int vbutton_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Input Demo Statistics\n");
	seq_printf(m, "=====================\n\n");
	seq_printf(m, "Device: %s\n", vbutton->name);
	seq_printf(m, "Presses:  %d\n", atomic_read(&press_count));
	seq_printf(m, "Releases: %d\n", atomic_read(&release_count));
	seq_printf(m, "\nCommands: press, release, click, power\n");
	return 0;
}

static int vbutton_open(struct inode *inode, struct file *file)
{
	return single_open(file, vbutton_show, NULL);
}

static const struct proc_ops vbutton_proc_ops = {
	.proc_open = vbutton_open,
	.proc_read = seq_read,
	.proc_write = vbutton_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static struct proc_dir_entry *proc_entry;

static int __init input_demo_init(void)
{
	int ret;

	/* Allocate input device */
	vbutton = input_allocate_device();
	if (!vbutton)
		return -ENOMEM;

	/* Set device metadata */
	vbutton->name = "Virtual Button Demo";
	vbutton->phys = "input_demo/input0";
	vbutton->id.bustype = BUS_VIRTUAL;
	vbutton->id.vendor = 0x0001;
	vbutton->id.product = 0x0001;
	vbutton->id.version = 0x0100;

	/* Declare capabilities */
	input_set_capability(vbutton, EV_KEY, KEY_ENTER);
	input_set_capability(vbutton, EV_KEY, KEY_POWER);

	/* Register input device */
	ret = input_register_device(vbutton);
	if (ret) {
		input_free_device(vbutton);
		pr_err("input_demo: failed to register input device\n");
		return ret;
	}

	/* Create proc entry for triggering events */
	proc_entry = proc_create("input_demo", 0666, NULL,
				 &vbutton_proc_ops);
	if (!proc_entry) {
		input_unregister_device(vbutton);
		return -ENOMEM;
	}

	pr_info("input_demo: registered virtual button\n");
	pr_info("input_demo: use /proc/input_demo or evtest\n");
	return 0;
}

static void __exit input_demo_exit(void)
{
	if (proc_entry)
		proc_remove(proc_entry);

	input_unregister_device(vbutton);

	pr_info("input_demo: unregistered\n");
}

module_init(input_demo_init);
module_exit(input_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Driver Development Guide");
MODULE_DESCRIPTION("Input subsystem demonstration");
MODULE_VERSION("1.0");
