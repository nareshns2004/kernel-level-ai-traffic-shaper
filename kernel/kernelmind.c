// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/init.h>

#include "kernelmind.h"
#include "flow_table.h"
#include "inference.h"
#include "policy.h"
#include "netlink.h"

#define MODULE_VERSION "0.1.0"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KernelMind Contributors");
MODULE_DESCRIPTION("AI-driven kernel-level network traffic shaper");
MODULE_VERSION(MODULE_VERSION);

static char *interface = "eth0";
module_param(interface, charp, 0644);
MODULE_PARM_DESC(interface, "Primary interface to shape");

static bool monitor_only;
module_param(monitor_only, bool, 0644);
MODULE_PARM_DESC(monitor_only, "Monitor-only mode (no shaping)");

static struct nf_hook_ops kernelmind_nf_ops[] = {
	{
		.hook     = kernelmind_nf_hook,
		.pf       = NFPROTO_IPV4,
		.hooknum  = NF_INET_PRE_ROUTING,
		.priority = NF_IP_PRI_FIRST,
	},
	{
		.hook     = kernelmind_nf_hook,
		.pf       = NFPROTO_IPV4,
		.hooknum  = NF_INET_POST_ROUTING,
		.priority = NF_IP_PRI_LAST,
	},
};

static int __init kernelmind_init(void)
{
	int ret;

	pr_info("kernelmind: loading v%s on %s\n", MODULE_VERSION, interface);

	ret = kernelmind_flow_table_init();
	if (ret)
		return ret;

	ret = kernelmind_inference_init();
	if (ret)
		goto err_flow;

	ret = kernelmind_policy_init();
	if (ret)
		goto err_infer;

	ret = kernelmind_netlink_init();
	if (ret)
		goto err_policy;

	ret = kernelmind_proc_init();
	if (ret)
		goto err_nl;

	ret = kernelmind_tc_init(interface);
	if (ret)
		pr_warn("kernelmind: tc integration unavailable (%d)\n", ret);

	ret = nf_register_net_hooks(&init_net, kernelmind_nf_ops,
				    ARRAY_SIZE(kernelmind_nf_ops));
	if (ret) {
		pr_err("kernelmind: failed to register netfilter hooks\n");
		goto err_proc;
	}

	pr_info("kernelmind: loaded successfully\n");
	return 0;

err_proc:
	kernelmind_proc_exit();
err_nl:
	kernelmind_netlink_exit();
err_policy:
	kernelmind_policy_exit();
err_infer:
	kernelmind_inference_exit();
err_flow:
	kernelmind_flow_table_exit();
	return ret;
}

static void __exit kernelmind_exit(void)
{
	nf_unregister_net_hooks(&init_net, kernelmind_nf_ops,
				ARRAY_SIZE(kernelmind_nf_ops));
	kernelmind_tc_exit();
	kernelmind_proc_exit();
	kernelmind_netlink_exit();
	kernelmind_policy_exit();
	kernelmind_inference_exit();
	kernelmind_flow_table_exit();
	pr_info("kernelmind: unloaded\n");
}

module_init(kernelmind_init);
module_exit(kernelmind_exit);
