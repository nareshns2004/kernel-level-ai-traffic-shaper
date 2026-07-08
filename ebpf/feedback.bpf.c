// SPDX-License-Identifier: GPL-2.0
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include "common.h"

char LICENSE[] SEC("license") = "GPL";

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 1024);
	__type(key, struct kernelmind_flow_key);
	__type(value, struct kernelmind_feedback);
} feedback_map SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, KERNELMIND_MAX_TREES * KERNELMIND_MAX_TREE_DEPTH);
	__type(key, __u32);
	__type(value, struct kernelmind_tree_node);
} model_weights SEC(".maps");

SEC("tracepoint/feedback")
int kernelmind_feedback(struct kernelmind_feedback *fb)
{
	struct kernelmind_feedback *existing;
	__u32 leaf_idx = 0;
	struct kernelmind_tree_node *node;

	if (!fb)
		return 0;

	existing = bpf_map_lookup_elem(&feedback_map, &fb->key);
	if (existing) {
		existing->reward_q8 = fb->reward_q8;
		existing->packet_loss = fb->packet_loss;
		existing->rtt_us = fb->rtt_us;
	} else {
		bpf_map_update_elem(&feedback_map, &fb->key, fb, BPF_ANY);
	}

	node = bpf_map_lookup_elem(&model_weights, &leaf_idx);
	if (node && node->is_leaf) {
		int delta = fb->reward_q8 >> 2;

		node->leaf_value_q8 += (__s16)delta;
		bpf_map_update_elem(&model_weights, &leaf_idx, node, BPF_EXIST);
	}

	return 0;
}
