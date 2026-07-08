// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/spinlock.h>
#include <linux/jhash.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include "flow_table.h"
#include "../ebpf/common.h"

#define FLOW_HASH_BITS 16

struct flow_entry {
	struct hlist_node node;
	struct kernelmind_flow_key key;
	struct kernelmind_flow_stats stats;
};

static DEFINE_HASHTABLE(flow_hash, FLOW_HASH_BITS);
static DEFINE_SPINLOCK(flow_lock);
static atomic_t flow_count = ATOMIC_INIT(0);

static u32 flow_hash_key(struct kernelmind_flow_key *key)
{
	return jhash(key, sizeof(*key), 0);
}

int kernelmind_flow_table_init(void)
{
	hash_init(flow_hash);
	atomic_set(&flow_count, 0);
	return 0;
}

void kernelmind_flow_table_exit(void)
{
	struct flow_entry *entry;
	struct hlist_node *tmp;
	int bkt;

	spin_lock(&flow_lock);
	hash_for_each_safe(flow_hash, bkt, tmp, entry, node) {
		hash_del(&entry->node);
		kfree(entry);
	}
	spin_unlock(&flow_lock);
}

struct kernelmind_flow_stats *
kernelmind_flow_lookup(struct kernelmind_flow_key *key)
{
	struct flow_entry *entry;
	u32 hash = flow_hash_key(key);

	rcu_read_lock();
	hash_for_each_possible_rcu(flow_hash, entry, node, hash) {
		if (!memcmp(&entry->key, key, sizeof(*key))) {
			rcu_read_unlock();
			return &entry->stats;
		}
	}
	rcu_read_unlock();
	return NULL;
}

static u16 compute_entropy(struct sk_buff *skb)
{
	u16 entropy = 0;
	u8 *data;
	int i, len;

	if (!skb->data || skb->len < 20)
		return 0;

	len = min_t(int, skb->len, 64);
	data = skb->data;
	for (i = 0; i < len; i++)
		entropy ^= (data[i] << (i & 7));

	return entropy;
}

int kernelmind_flow_update(struct kernelmind_flow_key *key,
			   struct sk_buff *skb, u64 now_ns)
{
	struct flow_entry *entry;
	u32 hash = flow_hash_key(key);
	u32 delta_us;

	spin_lock(&flow_lock);
	hash_for_each_possible(flow_hash, entry, node, hash) {
		if (memcmp(&entry->key, key, sizeof(*key)) == 0)
			goto update;
	}

	if (atomic_read(&flow_count) >= KERNELMIND_FLOW_TABLE_SIZE) {
		spin_unlock(&flow_lock);
		return -ENOSPC;
	}

	entry = kzalloc(sizeof(*entry), GFP_ATOMIC);
	if (!entry) {
		spin_unlock(&flow_lock);
		return -ENOMEM;
	}

	entry->key = *key;
	entry->stats.class_id = KM_CLASS_UNKNOWN;
	hash_add(flow_hash, &entry->node, hash);
	atomic_inc(&flow_count);

update:
	if (entry->stats.last_seen_ns) {
		delta_us = (u32)((now_ns - entry->stats.last_seen_ns) / 1000);
		entry->stats.inter_arrival_us = delta_us;
		if (delta_us > entry->stats.jitter_us)
			entry->stats.jitter_us = delta_us;
	}

	entry->stats.packets++;
	entry->stats.bytes += skb->len;
	entry->stats.last_seen_ns = now_ns;
	entry->stats.avg_pkt_size = (u16)(entry->stats.bytes /
					  entry->stats.packets);
	entry->stats.entropy_q8 = compute_entropy(skb);

	if (ip_hdr(skb)->protocol == IPPROTO_TCP) {
		struct tcphdr *tcph;

		if (pskb_may_pull(skb, ip_hdr(skb)->ihl * 4 +
				  sizeof(struct tcphdr))) {
			tcph = (struct tcphdr *)(skb_network_header(skb) +
						 ip_hdr(skb)->ihl * 4);
			entry->stats.tcp_flags_seen |= tcph->fin | tcph->syn |
						       tcph->rst | tcph->ack;
		}
	}

	spin_unlock(&flow_lock);
	return 0;
}

void kernelmind_flow_table_clear(void)
{
	struct flow_entry *entry;
	struct hlist_node *tmp;
	int bkt;

	spin_lock(&flow_lock);
	hash_for_each_safe(flow_hash, bkt, tmp, entry, node) {
		hash_del(&entry->node);
		kfree(entry);
	}
	atomic_set(&flow_count, 0);
	spin_unlock(&flow_lock);
}

u32 kernelmind_flow_count(void)
{
	return (u32)atomic_read(&flow_count);
}
