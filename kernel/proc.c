// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "flow_table.h"
#include "inference.h"
#include "../ebpf/common.h"

static struct proc_dir_entry *proc_root;

static int stats_show(struct seq_file *m, void *v)
{
	struct kernelmind_global_stats *stats = kernelmind_get_global_stats();

	seq_printf(m, "packets_processed\t%llu\n", stats->packets_processed);
	seq_printf(m, "flows_classified\t%llu\n", stats->flows_classified);
	seq_printf(m, "anomalies_detected\t%llu\n", stats->anomalies_detected);
	seq_printf(m, "packets_dropped\t%llu\n", stats->packets_dropped);
	seq_printf(m, "inference_count\t%llu\n", stats->inference_count);
	seq_printf(m, "inference_total_ns\t%llu\n",
		   stats->inference_total_ns);
	seq_printf(m, "active_flows\t%u\n", kernelmind_flow_count());
	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, stats_show, NULL);
}

static const struct proc_ops stats_ops = {
	.proc_open = stats_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int metrics_show(struct seq_file *m, void *v)
{
	struct kernelmind_global_stats *stats = kernelmind_get_global_stats();
	u64 avg_ns = 0;

	if (stats->inference_count)
		avg_ns = stats->inference_total_ns / stats->inference_count;

	seq_printf(m, "# HELP kernelmind_flows_classified_total "
		   "Total classified flows\n");
	seq_printf(m, "# TYPE kernelmind_flows_classified_total counter\n");
	seq_printf(m, "kernelmind_flows_classified_total %llu\n",
		   stats->flows_classified);
	seq_printf(m, "# HELP kernelmind_inference_latency_ns "
		   "Average inference latency\n");
	seq_printf(m, "# TYPE kernelmind_inference_latency_ns gauge\n");
	seq_printf(m, "kernelmind_inference_latency_ns %llu\n", avg_ns);
	seq_printf(m, "# HELP kernelmind_active_flows "
		   "Current active flow count\n");
	seq_printf(m, "# TYPE kernelmind_active_flows gauge\n");
	seq_printf(m, "kernelmind_active_flows %u\n", kernelmind_flow_count());
	return 0;
}

static int metrics_open(struct inode *inode, struct file *file)
{
	return single_open(file, metrics_show, NULL);
}

static const struct proc_ops metrics_ops = {
	.proc_open = metrics_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int kernelmind_proc_init(void)
{
	proc_root = proc_mkdir("kernelmind", NULL);
	if (!proc_root)
		return -ENOMEM;

	proc_create("stats", 0444, proc_root, &stats_ops);
	proc_create("metrics", 0444, proc_root, &metrics_ops);
	return 0;
}

void kernelmind_proc_exit(void)
{
	remove_proc_subtree("kernelmind", NULL);
	proc_root = NULL;
}
