/* SPDX-License-Identifier: MIT */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define MAX_FEATURES 32
#define MAX_DEPTH 8
#define MAX_TREES 64

typedef struct {
	int16_t values[MAX_FEATURES];
	uint8_t count;
} feature_vec_t;

typedef struct {
	int16_t feature_idx;
	int16_t threshold_q8;
	int16_t left;
	int16_t right;
	int16_t leaf_value_q8;
	uint8_t is_leaf;
} tree_node_t;

static tree_node_t model[MAX_TREES * MAX_DEPTH];
static size_t model_nodes = MAX_TREES * MAX_DEPTH;

static int tree_predict(uint32_t tree_idx, feature_vec_t *feat)
{
	uint32_t node_idx = tree_idx * MAX_DEPTH;
	uint32_t i;
	int score = 0;

	for (i = 0; i < MAX_DEPTH; i++) {
		tree_node_t *node = &model[node_idx];

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

static uint64_t nsec_now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main(void)
{
	feature_vec_t feat = { .count = 8 };
	uint64_t start, end, total = 0;
	uint64_t samples[10000];
	size_t i, t;
	int j;

	for (i = 0; i < model_nodes; i++) {
		model[i].is_leaf = 1;
		model[i].leaf_value_q8 = (int16_t)(i % 12);
	}

	for (j = 0; j < 8; j++)
		feat.values[j] = (int16_t)(100 + j * 50);

	for (i = 0; i < 10000; i++) {
		start = nsec_now();
		for (t = 0; t < MAX_TREES; t++)
			(void)tree_predict(t, &feat);
		end = nsec_now();
		samples[i] = end - start;
		total += samples[i];
	}

	/* Simple sort for p50/p99 */
	for (i = 0; i < 10000; i++) {
		for (size_t k = i + 1; k < 10000; k++) {
			if (samples[k] < samples[i]) {
				uint64_t tmp = samples[i];
				samples[i] = samples[k];
				samples[k] = tmp;
			}
		}
	}

	printf("inference bench (userspace stub)\n");
	printf("samples:     10000\n");
	printf("p50_ns:      %" PRIu64 "\n", samples[5000]);
	printf("p99_ns:      %" PRIu64 "\n", samples[9900]);
	printf("avg_ns:      %" PRIu64 "\n", total / 10000);
	return 0;
}
