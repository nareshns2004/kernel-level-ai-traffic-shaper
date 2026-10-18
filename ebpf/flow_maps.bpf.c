// SPDX-License-Identifier: GPL-2.0
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
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

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 1024);
	__type(key, struct kernelmind_flow_key);
	__type(value, struct kernelmind_feedback);
} feedback_map SEC(".maps");
