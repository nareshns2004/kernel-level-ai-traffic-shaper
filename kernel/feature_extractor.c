// SPDX-License-Identifier: GPL-2.0
#include "feature_extractor.h"
#include "../ebpf/common.h"

void kernelmind_extract_features(struct kernelmind_flow_stats *stats,
				 struct kernelmind_feature_vec *feat)
{
	if (!stats || !feat)
		return;

	memset(feat, 0, sizeof(*feat));
	feat->count = 8;
	feat->values[0] = (__s16)min_t(u32, stats->inter_arrival_us, 32767);
	feat->values[1] = (__s16)min_t(u32, stats->jitter_us, 32767);
	feat->values[2] = (__s16)stats->avg_pkt_size;
	feat->values[3] = (__s16)stats->entropy_q8;
	feat->values[4] = (__s16)min_t(u64, stats->bytes >> 10, 32767);
	feat->values[5] = (__s16)min_t(u64, stats->packets, 32767);
	feat->values[6] = (__s16)stats->tcp_flags_seen;
	feat->values[7] = 0;
}
