/*
 * Copyright (C) 2020 Amazon.com, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/atomic.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/atomic.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <asm-generic/sizes.h>
#include <linux/uaccess.h>
#include <asm/timex.h>
#include <linux/kallsyms.h>
#include <linux/debugfs.h>
#include <linux/amzn_rt_trace.h>

#define AMZN_TRACE_COMPAT_STR "amzn,rt-trace"
#define AMZN_TRACE_MAGIC0 0x414d /* "AM" */
#define AMZN_TRACE_MAGIC1 0xbeb2 /* ~"AM" */
struct amzn_rt_trace_item {
	u16 magic[2];
	u32 id;
	char tag[4];
	u32 cpu;
	u64 timestamp_nsec;
	u64 caller;
	u64 value1;
	u64 value2;
} __attribute__ ((__packed__));

#define TRACE_TYPE_NAME_MAX 16
struct amzn_rt_trace_stat {
	struct amzn_rt_trace_item *trace;
	phys_addr_t phys;
	char name[TRACE_TYPE_NAME_MAX];
	u32 size;
	u32 entries;
	atomic_t idx;
	int enabled;
	struct dentry *dent;
};

struct amzn_rt_trace_stat *amzn_rt_trace_data[AMZN_RT_TRACE_TYPE_MAX + 1] = {0};

static int amzn_rt_trace_panic_notifier(struct notifier_block *this,
					unsigned long event, void *ptr)
{
	u32 type;

	for (type = 0; type <= AMZN_RT_TRACE_TYPE_MAX; type++)
		amzn_rt_trace_data[type]->enabled = 0;

	return NOTIFY_DONE;
}

static struct notifier_block amzn_rt_trace_panic_blk = {
	.notifier_call  = amzn_rt_trace_panic_notifier,
	.priority = INT_MAX,
};

void notrace amzn_rt_trace_tag(enum amzn_rt_trace_type type,
			       char *tag, u64 value1, u64 value2)
{
	u32 idx;
	struct amzn_rt_trace_item *item;
	struct amzn_rt_trace_stat *trace_stat;

	if (type < 0 || type > AMZN_RT_TRACE_TYPE_MAX)
		return;

	trace_stat = amzn_rt_trace_data[type];
	if (!trace_stat || !trace_stat->enabled)
		return;

	idx = atomic_inc_return(&trace_stat->idx) - 1;
	idx = idx & (trace_stat->entries - 1);

	item = &trace_stat->trace[idx];
	item->magic[0] = AMZN_TRACE_MAGIC0;
	item->magic[1] = AMZN_TRACE_MAGIC1;
	item->id = idx;
	if (tag) {
		item->tag[0] = tag[0];
		item->tag[1] = tag[1];
		item->tag[2] = tag[2];
		item->tag[3] = tag[3];
	} else {
		item->tag[0] = '\0';
	}
	item->caller = (u64)__builtin_return_address(0);
	item->value1 = value1;
	item->value2 = value2;
	item->timestamp_nsec = sched_clock();
	item->cpu = raw_smp_processor_id();

	/* memory barrier */
	mb();
}
EXPORT_SYMBOL(amzn_rt_trace_tag);

noinline void notrace amzn_rt_trace(enum amzn_rt_trace_type type,
				    u64 value1, u64 value2)
{
	amzn_rt_trace_tag(type, NULL, value1, value2);
}
EXPORT_SYMBOL(amzn_rt_trace);

static int amzn_rt_trace_show(struct seq_file *file, void *data)
{
	char func_name[100], tag[5];
	struct amzn_rt_trace_stat *trace_stat = file->private;
	unsigned long msec, rem_nsec;
	struct amzn_rt_trace_item *item;
	u32 idx_free;
	u32 idx, first = 1;

	if (!trace_stat)
		return -EINVAL;

	idx_free = atomic_read(&trace_stat->idx) & (trace_stat->entries - 1);
	idx = idx_free;

	seq_printf(file, "%-14s %s %-4s %-36s %-36s %s\n",
		   "Timestamp", "CPU", "Tag",
		   "Value1(d)", "Value2(d)", "Caller");

	while (first || idx != idx_free) {
		first = 0;
		item = &trace_stat->trace[idx];
		idx = (idx + 1) & (trace_stat->entries - 1);
		if (item->magic[0] != AMZN_TRACE_MAGIC0 ||
		    item->magic[1] != AMZN_TRACE_MAGIC1)
			continue;

		msec = item->timestamp_nsec / 1000000000;
		rem_nsec = item->timestamp_nsec % 1000000000;
		memset(func_name, 0, sizeof(func_name));
		sprint_symbol(func_name, item->caller);
		memcpy(tag, item->tag, 4);
		tag[4] = '\0';
		seq_printf(file, "[%5lu.%06lu] <%d> %s 0x%016x(%16d) 0x%016x(%16d) %s\n",
			   msec,
			   rem_nsec / 1000,
			   item->cpu,
			   tag,
			   item->value1,
			   item->value1,
			   item->value2,
			   item->value2,
			   func_name);
	}

	return 0;
}

static int amzn_rt_trace_open(struct inode *inode, struct file *file)
{
	return single_open(file, amzn_rt_trace_show, inode->i_private);
}

static const struct file_operations amzn_rt_trace_dbg_fops = {
	.open	= amzn_rt_trace_open,
	.read	= seq_read,
};

static int amzn_rt_trace_debugfs_init(void)
{
	struct dentry *root;
	struct amzn_rt_trace_stat *trace_stat;
	u32 type;

	root = debugfs_create_dir(AMZN_TRACE_COMPAT_STR, NULL);
	if (!root)
		return -ENOMEM;

	for (type = 0; type <= AMZN_RT_TRACE_TYPE_MAX; type++) {
		trace_stat = amzn_rt_trace_data[type];
		trace_stat->dent = debugfs_create_file(trace_stat->name,
						       S_IRUSR, root,
						       trace_stat,
						       &amzn_rt_trace_dbg_fops);
		if (!trace_stat->dent)
			goto fail;
	}

	return 0;
fail:
	debugfs_remove_recursive(root);
	return -ENOMEM;
}

static int amzn_rt_trace_probe(struct platform_device *pdev)
{
	int ret = 0;
	u32 type, count;
	struct device_node *node;
	struct amzn_rt_trace_stat *trace_stat;

	for_each_child_of_node(pdev->dev.of_node, node) {
		ret = of_property_read_u32(node, "type", (u32 *)&type);
		if (ret < 0 || type < 0 || type > AMZN_RT_TRACE_TYPE_MAX ||
		    amzn_rt_trace_data[type]) {
			ret = -EINVAL;
			goto fail;
		}

		ret = of_property_read_u32(node, "trace-items", (u32 *)&count);
		if (ret < 0 || count <= 0) {
			ret = -EINVAL;
			goto fail;
		}

		trace_stat = kmalloc(sizeof(struct amzn_rt_trace_stat),
				     GFP_KERNEL);
		if (!trace_stat)
			goto fail;
		amzn_rt_trace_data[type] = trace_stat;

		strlcpy(trace_stat->name, node->name, TRACE_TYPE_NAME_MAX);
		/* Round this down to a power of 2 */
		trace_stat->entries = __rounddown_pow_of_two(count);
		trace_stat->size = trace_stat->entries
				   * sizeof(struct amzn_rt_trace_item);

		pr_info("%s: name = %s, size = 0x%x, entries = %d\n",
			__func__,
			trace_stat->name,
			trace_stat->size,
			trace_stat->entries);

		trace_stat->trace = dma_alloc_coherent(&pdev->dev,
						       trace_stat->size,
						       &trace_stat->phys,
						       GFP_KERNEL);
		if (!trace_stat->trace) {
			ret = -ENOMEM;
			goto fail;
		}

		memset(trace_stat->trace, 0, trace_stat->size);
		atomic_set(&trace_stat->idx, 0);
		trace_stat->enabled = 1;
	}

	if (amzn_rt_trace_debugfs_init())
		goto fail;

	atomic_notifier_chain_register(&panic_notifier_list,
				       &amzn_rt_trace_panic_blk);

	return 0;
fail:
	for (type = 0; type <= AMZN_RT_TRACE_TYPE_MAX; type++) {
		trace_stat = amzn_rt_trace_data[type];
		if (trace_stat) {
			if (trace_stat->trace)
				dma_free_coherent(&pdev->dev,
						  trace_stat->size,
						  trace_stat->trace,
						  trace_stat->phys);
			kfree(trace_stat);
		}
	}
	return ret;
}

static const struct of_device_id amzn_rt_trace_match_table[] = {
	{.compatible = AMZN_TRACE_COMPAT_STR},
	{},
};

static struct platform_driver amzn_rt_trace_driver = {
	.driver         = {
		.name = "amzn_rt_trace",
		.owner = THIS_MODULE,
		.of_match_table = amzn_rt_trace_match_table
	},
};

static int __init amzn_rt_trace_init(void)
{
	return platform_driver_probe(&amzn_rt_trace_driver,
				     amzn_rt_trace_probe);
}

rootfs_initcall(amzn_rt_trace_init)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amazon");
MODULE_DESCRIPTION("Memory based simple tracer");
