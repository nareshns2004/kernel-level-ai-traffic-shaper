// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>
#include <linux/hashtable.h>
#include <linux/jhash.h>

#include "policy.h"
#include "../ebpf/common.h"

#define OVERRIDE_HASH_BITS 8

struct override_entry {
	struct hlist_node node;
	struct kernelmind_policy_entry entry;
};

static DEFINE_HASHTABLE(override_hash, OVERRIDE_HASH_BITS);
static DEFINE_SPINLOCK(override_lock);

static u32 class_to_mark(u8 class_id)
{
	switch (class_id) {
	case KM_CLASS_REALTIME:
		return 0x100;
	case KM_CLASS_INTERACTIVE:
		return 0x200;
	case KM_CLASS_GAMING:
		return 0x300;
	case KM_CLASS_VOIP:
		return 0x400;
	case KM_CLASS_STREAMING:
		return 0x500;
	case KM_CLASS_ANOMALY:
		return 0xF00;
	default:
		return 0x600;
	}
}

int kernelmind_policy_init(void)
{
	hash_init(override_hash);
	return 0;
}

void kernelmind_policy_exit(void)
{
	struct override_entry *entry;
	struct hlist_node *tmp;
	int bkt;

	spin_lock(&override_lock);
	hash_for_each_safe(override_hash, bkt, tmp, entry, node) {
		hash_del(&entry->node);
		kfree(entry);
	}
	spin_unlock(&override_lock);
}

unsigned int kernelmind_apply_policy(struct sk_buff *skb,
				     struct kernelmind_inference_result *result)
{
	u8 class_id = result->class_id;

	if (class_id == KM_CLASS_ANOMALY)
		return NF_DROP;

	skb->mark = class_to_mark(class_id);
	return NF_ACCEPT;
}

int kernelmind_policy_add_override(struct kernelmind_flow_key *key,
				   u8 class_id, u64 duration_ns)
{
	struct override_entry *entry;
	u32 hash = jhash(key, sizeof(*key), 0);

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	entry->entry.key = *key;
	entry->entry.class_id = class_id;
	entry->entry.expires_ns = ktime_get_ns() + duration_ns;

	spin_lock(&override_lock);
	hash_add(override_hash, &entry->node, hash);
	spin_unlock(&override_lock);

	return 0;
}

void kernelmind_policy_clear_overrides(void)
{
	struct override_entry *entry;
	struct hlist_node *tmp;
	int bkt;

	spin_lock(&override_lock);
	hash_for_each_safe(override_hash, bkt, tmp, entry, node) {
		hash_del(&entry->node);
		kfree(entry);
	}
	spin_unlock(&override_lock);
}
