// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/slab.h>

#include "netlink.h"
#include "flow_table.h"
#include "inference.h"
#include "../ebpf/common.h"

static struct sock *nl_sock;

static void kernelmind_nl_reply(struct sk_buff *skb, struct nlmsghdr *nlh,
				 void *data, int len)
{
	struct sk_buff *msg;
	struct nlmsghdr *rep;

	msg = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!msg)
		return;

	rep = nlmsg_put(msg, NETLINK_CB(skb).portid, nlh->nlmsg_seq,
			NLMSG_DONE, len, 0);
	if (!rep) {
		kfree_skb(msg);
		return;
	}

	if (len && data)
		memcpy(nlmsg_data(rep), data, len);

	netlink_unicast(nl_sock, msg, NETLINK_CB(skb).portid, MSG_DONTWAIT);
}

static void kernelmind_nl_handler(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	struct kernelmind_global_stats *stats;
	int cmd;

	if (!skb)
		return;

	nlh = nlmsg_hdr(skb);
	cmd = nlh->nlmsg_type;

	switch (cmd) {
	case KERNELMIND_NL_CMD_STATS:
		stats = kernelmind_get_global_stats();
		kernelmind_nl_reply(skb, nlh, stats, sizeof(*stats));
		break;
	case KERNELMIND_NL_CMD_FLOWS:
		kernelmind_nl_reply(skb, nlh, NULL, 0);
		break;
	case KERNELMIND_NL_CMD_RECLASSIFY:
		kernelmind_flow_table_clear();
		break;
	default:
		break;
	}
}

int kernelmind_netlink_init(void)
{
	struct netlink_kernel_cfg cfg = {
		.input = kernelmind_nl_handler,
	};

	nl_sock = netlink_kernel_create(&init_net, KERNELMIND_NETLINK_FAMILY,
					&cfg);
	if (!nl_sock)
		return -ENOMEM;

	return 0;
}

void kernelmind_netlink_exit(void)
{
	if (nl_sock) {
		netlink_kernel_release(nl_sock);
		nl_sock = NULL;
	}
}

void kernelmind_netlink_broadcast_stats(void)
{
	struct kernelmind_global_stats *stats;

	if (!nl_sock)
		return;

	stats = kernelmind_get_global_stats();
	netlink_broadcast(nl_sock, NULL, 0, 0, GFP_KERNEL);
}
