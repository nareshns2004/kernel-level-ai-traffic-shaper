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
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 4096);
	__type(key, struct kernelmind_flow_key);
	__type(value, struct kernelmind_inference_result);
} policy_cache SEC(".maps");

SEC("classifier")
int kernelmind_tc_classify(struct __sk_buff *skb)
{
	struct kernelmind_flow_key key = {};
	struct kernelmind_inference_result *result;
	void *data = (void *)(long)skb->data;
	void *data_end = (void *)(long)skb->data_end;
	struct {
		__u8  ver_ihl;
		__u8  tos;
		__u16 tot_len;
		__u16 id;
		__u16 frag_off;
		__u8  ttl;
		__u8  protocol;
		__u16 check;
		__u32 saddr;
		__u32 daddr;
	} *ip;

	ip = data;
	if ((void *)(ip + 1) > data_end)
		return 0;

	key.src_ip = ip->saddr;
	key.dst_ip = ip->daddr;
	key.proto = ip->protocol;

	result = bpf_map_lookup_elem(&policy_cache, &key);
	if (!result)
		return 0;

	skb->mark = result->class_id;
	return 0;
}
