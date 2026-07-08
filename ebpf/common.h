/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KERNELMIND_COMMON_H
#define __KERNELMIND_COMMON_H

#include <linux/types.h>

#define KERNELMIND_MODEL_VERSION	1
#define KERNELMIND_MAX_FEATURES		32
#define KERNELMIND_MAX_TREES		64
#define KERNELMIND_MAX_TREE_DEPTH	8
#define KERNELMIND_FLOW_TABLE_SIZE	65536
#define KERNELMIND_INFERENCE_THRESHOLD	8
#define KERNELMIND_NUM_CLASSES		12

#define KERNELMIND_NETLINK_FAMILY	31

enum kernelmind_flow_class {
	KM_CLASS_UNKNOWN = 0,
	KM_CLASS_REALTIME,
	KM_CLASS_INTERACTIVE,
	KM_CLASS_STREAMING,
	KM_CLASS_BULK,
	KM_CLASS_P2P,
	KM_CLASS_GAMING,
	KM_CLASS_VOIP,
	KM_CLASS_WEB,
	KM_CLASS_DNS,
	KM_CLASS_ANOMALY,
	KM_CLASS_BACKGROUND,
	KM_CLASS_MAX,
};

enum kernelmind_policy_action {
	KM_ACTION_ACCEPT = 0,
	KM_ACTION_MARK,
	KM_ACTION_THROTTLE,
	KM_ACTION_DROP,
	KM_ACTION_REMARK_DSCP,
};

struct kernelmind_flow_key {
	__u32 src_ip;
	__u32 dst_ip;
	__u16 src_port;
	__u16 dst_port;
	__u8  proto;
	__u8  pad[3];
} __attribute__((packed));

struct kernelmind_flow_stats {
	__u64 packets;
	__u64 bytes;
	__u64 last_seen_ns;
	__u32 inter_arrival_us;
	__u32 jitter_us;
	__u16 avg_pkt_size;
	__u16 entropy_q8;
	__u8  tcp_flags_seen;
	__u8  class_id;
	__u8  priority_q8;
	__u8  flags;
};

struct kernelmind_feature_vec {
	__s16 values[KERNELMIND_MAX_FEATURES];
	__u8  count;
	__u8  pad[3];
} __attribute__((packed));

struct kernelmind_inference_result {
	__u8  class_id;
	__u8  priority_q8;
	__u16 predicted_mbps_q8;
	__u32 confidence_q8;
};

struct kernelmind_tree_node {
	__s16 feature_idx;
	__s16 threshold_q8;
	__s16 left;
	__s16 right;
	__s16 leaf_value_q8;
	__u8  is_leaf;
	__u8  pad;
} __attribute__((packed));

struct kernelmind_model_header {
	__u32 version;
	__u32 num_trees;
	__u32 num_features;
	__u32 num_classes;
	__u32 tree_offset;
	__u32 checksum;
};

struct kernelmind_policy_entry {
	struct kernelmind_flow_key key;
	__u8  class_id;
	__u8  action;
	__u16 mark;
	__u32 rate_kbps;
	__u64 expires_ns;
};

struct kernelmind_feedback {
	struct kernelmind_flow_key key;
	__s16 reward_q8;
	__u8  packet_loss;
	__u8  pad;
	__u32 rtt_us;
};

struct kernelmind_global_stats {
	__u64 packets_processed;
	__u64 flows_classified;
	__u64 anomalies_detected;
	__u64 packets_dropped;
	__u64 inference_count;
	__u64 inference_total_ns;
};

#endif /* __KERNELMIND_COMMON_H */
