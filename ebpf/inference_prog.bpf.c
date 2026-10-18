// SPDX-License-Identifier: GPL-2.0
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include "common.h"

char LICENSE[] SEC("license") = "GPL";

struct {
	__uint(type, BPF_MAP_TYPE_LRU_HASH);
	__uint(max_entries, KERNELMIND_FLOW_TABLE_SIZE);
	__type(key, struct kernelmind_flow_key);
	__type(value, struct kernelmind_flow_stats);
} flow_table SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, KERNELMIND_MAX_TREES * KERNELMIND_MAX_TREE_DEPTH);
	__type(key, __u32);
	__type(value, struct kernelmind_tree_node);
} model_weights SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 4096);
	__type(key, struct kernelmind_flow_key);
	__type(value, struct kernelmind_inference_result);
} policy_cache SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct kernelmind_global_stats);
} global_stats SEC(".maps");

static __always_inline int
km_tree_predict(__u32 tree_idx, struct kernelmind_feature_vec *feat)
{
	__u32 node_idx = tree_idx * KERNELMIND_MAX_TREE_DEPTH;
	__u32 i;
	int score = 0;

	for (i = 0; i < KERNELMIND_MAX_TREE_DEPTH; i++) {
		struct kernelmind_tree_node *node;

		node = bpf_map_lookup_elem(&model_weights, &node_idx);
		if (!node)
			return 0;

		if (node->is_leaf) {
			score += node->leaf_value_q8;
			break;
		}

		if (node->feature_idx >= feat->count)
			return score;

		if (feat->values[node->feature_idx] <= node->threshold_q8)
			node_idx = node->left;
		else
			node_idx = node->right;
	}

	return score;
}

static __always_inline void
km_build_features(struct kernelmind_flow_stats *stats,
		  struct kernelmind_feature_vec *feat)
{
	feat->count = 8;
	feat->values[0] = (__s16)stats->inter_arrival_us;
	feat->values[1] = (__s16)stats->jitter_us;
	feat->values[2] = (__s16)stats->avg_pkt_size;
	feat->values[3] = (__s16)stats->entropy_q8;
	feat->values[4] = (__s16)(stats->bytes >> 10);
	feat->values[5] = (__s16)stats->packets;
	feat->values[6] = (__s16)stats->tcp_flags_seen;
	feat->values[7] = 0;
}

SEC("classifier/inference")
int kernelmind_infer(struct kernelmind_flow_key *key)
{
	struct kernelmind_flow_stats *stats;
	struct kernelmind_feature_vec feat = {};
	struct kernelmind_inference_result result = {};
	__u32 stats_key = 0;
	struct kernelmind_global_stats *gstats;
	int total_score = 0;
	__u32 t;

	stats = bpf_map_lookup_elem(&flow_table, key);
	if (!stats)
		return 0;

	if (stats->packets < KERNELMIND_INFERENCE_THRESHOLD)
		return 0;

	km_build_features(stats, &feat);

	for (t = 0; t < KERNELMIND_MAX_TREES; t++)
		total_score += km_tree_predict(t, &feat);

	result.class_id = (__u8)((total_score >> 8) % KERNELMIND_NUM_CLASSES);
	if (result.class_id == 0)
		result.class_id = KM_CLASS_UNKNOWN;

	result.priority_q8 = (__u8)((total_score >> 4) & 0xFF);
	result.predicted_mbps_q8 = (__u16)(stats->avg_pkt_size * 8);
	result.confidence_q8 = (__u32)(stats->packets > 16 ? 220 : 128);

	bpf_map_update_elem(&policy_cache, key, &result, BPF_ANY);

	stats->class_id = result.class_id;
	stats->priority_q8 = result.priority_q8;

	gstats = bpf_map_lookup_elem(&global_stats, &stats_key);
	if (gstats) {
		__sync_fetch_and_add(&gstats->flows_classified, 1);
		__sync_fetch_and_add(&gstats->inference_count, 1);
	}

	return 0;
}
