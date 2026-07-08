// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ktime.h>

#include "kernelmind.h"
#include "flow_table.h"
#include "inference.h"
#include "policy.h"
#include "../ebpf/common.h"

extern bool monitor_only;

static void kernelmind_build_key(struct sk_buff *skb,
				 struct kernelmind_flow_key *key)
{
	struct iphdr *iph = ip_hdr(skb);

	key->src_ip = iph->saddr;
	key->dst_ip = iph->daddr;
	key->proto = iph->protocol;

	if (iph->protocol == IPPROTO_TCP) {
		struct tcphdr *tcph;

		if (!pskb_may_pull(skb, iph->ihl * 4 + sizeof(struct tcphdr)))
			return;
		tcph = (struct tcphdr *)(skb_network_header(skb) +
					 iph->ihl * 4);
		key->src_port = ntohs(tcph->source);
		key->dst_port = ntohs(tcph->dest);
	} else if (iph->protocol == IPPROTO_UDP) {
		struct udphdr *udph;

		if (!pskb_may_pull(skb, iph->ihl * 4 + sizeof(struct udphdr)))
			return;
		udph = (struct udphdr *)(skb_network_header(skb) +
					 iph->ihl * 4);
		key->src_port = ntohs(udph->source);
		key->dst_port = ntohs(udph->dest);
	} else {
		key->src_port = 0;
		key->dst_port = 0;
	}
}

unsigned int kernelmind_nf_hook(void *priv, struct sk_buff *skb,
				const struct nf_hook_state *state)
{
	struct kernelmind_flow_key key = {};
	struct kernelmind_flow_stats *stats;
	struct kernelmind_inference_result result = {};
	u64 now_ns = ktime_get_ns();
	unsigned int verdict = NF_ACCEPT;

	if (!skb || !skb->dev)
		return NF_ACCEPT;

	if (ip_hdr(skb)->version != 4)
		return NF_ACCEPT;

	kernelmind_build_key(skb, &key);

	if (kernelmind_flow_update(&key, skb, now_ns))
		return NF_ACCEPT;

	stats = kernelmind_flow_lookup(&key);
	if (!stats)
		return NF_ACCEPT;

	if (stats->packets >= KERNELMIND_INFERENCE_THRESHOLD &&
	    stats->class_id == KM_CLASS_UNKNOWN) {
		if (!kernelmind_classify(&key, stats, &result)) {
			stats->class_id = result.class_id;
			stats->priority_q8 = result.priority_q8;
		}
	}

	if (!monitor_only && stats->class_id != KM_CLASS_UNKNOWN) {
		result.class_id = stats->class_id;
		result.priority_q8 = stats->priority_q8;
		verdict = kernelmind_apply_policy(skb, &result);
	}

	return verdict;
}
