/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KERNELMIND_INFERENCE_H
#define __KERNELMIND_INFERENCE_H

#include "../ebpf/common.h"

int kernelmind_inference_init(void);
void kernelmind_inference_exit(void);
int kernelmind_classify(struct kernelmind_flow_key *key,
			struct kernelmind_flow_stats *stats,
			struct kernelmind_inference_result *result);
struct kernelmind_global_stats *kernelmind_get_global_stats(void);
int kernelmind_model_import(void *data, size_t len);

#endif /* __KERNELMIND_INFERENCE_H */
