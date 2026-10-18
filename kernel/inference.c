// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/ktime.h>

#include "inference.h"
#include "feature_extractor.h"
#include "../ebpf/common.h"

static struct kernelmind_tree_node *model_nodes;
static u32 model_node_count;
static struct kernelmind_global_stats gstats;

static int km_tree_predict(u32 tree_idx, struct kernelmind_feature_vec *feat)
{
	u32 node_idx = tree_idx * KERNELMIND_MAX_TREE_DEPTH;
	u32 i;
	int score = 0;

	for (i = 0; i < KERNELMIND_MAX_TREE_DEPTH; i++) {
		struct kernelmind_tree_node *node;

		if (node_idx >= model_node_count)
			break;

		node = &model_nodes[node_idx];
		if (node->is_leaf) {
			score += node->leaf_value_q8;
			break;
		}

		if (node->feature_idx >= feat->count)
			break;

		if (feat->values[node->feature_idx] <= node->threshold_q8)
			node_idx = node->left;
		else
			node_idx = node->right;
	}

	return score;
}

int kernelmind_inference_init(void)
{
	u32 i;

	model_node_count = KERNELMIND_MAX_TREES * KERNELMIND_MAX_TREE_DEPTH;
	model_nodes = kcalloc(model_node_count, sizeof(*model_nodes),
			      GFP_KERNEL);
	if (!model_nodes)
		return -ENOMEM;

	for (i = 0; i < model_node_count; i++) {
		model_nodes[i].is_leaf = 1;
		model_nodes[i].leaf_value_q8 = (i % KERNELMIND_NUM_CLASSES) * 16;
	}

	memset(&gstats, 0, sizeof(gstats));
	return 0;
}

void kernelmind_inference_exit(void)
{
	kfree(model_nodes);
	model_nodes = NULL;
	model_node_count = 0;
}

int kernelmind_classify(struct kernelmind_flow_key *key,
			struct kernelmind_flow_stats *stats,
			struct kernelmind_inference_result *result)
{
	struct kernelmind_feature_vec feat;
	u64 start, elapsed;
	u32 t;
	int total_score = 0;

	if (!stats || !result)
		return -EINVAL;

	start = ktime_get_ns();
	kernelmind_extract_features(stats, &feat);

	for (t = 0; t < KERNELMIND_MAX_TREES; t++)
		total_score += km_tree_predict(t, &feat);

	result->class_id = (u8)((total_score >> 8) % KERNELMIND_NUM_CLASSES);
	if (result->class_id == 0)
		result->class_id = KM_CLASS_BULK;

	result->priority_q8 = (u8)((total_score >> 4) & 0xFF);
	result->predicted_mbps_q8 = stats->avg_pkt_size * 8;
	result->confidence_q8 = stats->packets > 16 ? 220 : 128;

	elapsed = ktime_get_ns() - start;
	gstats.inference_count++;
	gstats.inference_total_ns += elapsed;
	gstats.flows_classified++;

	return 0;
}

struct kernelmind_global_stats *kernelmind_get_global_stats(void)
{
	return &gstats;
}

int kernelmind_model_import(void *data, size_t len)
{
	struct kernelmind_model_header *hdr = data;

	if (len < sizeof(*hdr))
		return -EINVAL;

	if (hdr->version != KERNELMIND_MODEL_VERSION)
		return -EPROTO;

	return 0;
}
