/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KERNELMIND_POLICY_H
#define __KERNELMIND_POLICY_H

#include <linux/skbuff.h>
#include "../ebpf/common.h"

int kernelmind_policy_init(void);
void kernelmind_policy_exit(void);
unsigned int kernelmind_apply_policy(struct sk_buff *skb,
				     struct kernelmind_inference_result *result);
int kernelmind_policy_add_override(struct kernelmind_flow_key *key,
				   u8 class_id, u64 duration_ns);
void kernelmind_policy_clear_overrides(void);

#endif /* __KERNELMIND_POLICY_H */
