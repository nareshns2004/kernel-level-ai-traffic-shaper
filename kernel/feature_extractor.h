/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KERNELMIND_FEATURE_EXTRACTOR_H
#define __KERNELMIND_FEATURE_EXTRACTOR_H

#include "../ebpf/common.h"

void kernelmind_extract_features(struct kernelmind_flow_stats *stats,
				 struct kernelmind_feature_vec *feat);

#endif /* __KERNELMIND_FEATURE_EXTRACTOR_H */
