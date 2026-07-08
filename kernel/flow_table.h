/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KERNELMIND_FLOW_TABLE_H
#define __KERNELMIND_FLOW_TABLE_H

#include <linux/skbuff.h>
#include "../ebpf/common.h"

int kernelmind_flow_table_init(void);
void kernelmind_flow_table_exit(void);
struct kernelmind_flow_stats *
kernelmind_flow_lookup(struct kernelmind_flow_key *key);
int kernelmind_flow_update(struct kernelmind_flow_key *key,
			   struct sk_buff *skb, u64 now_ns);
void kernelmind_flow_table_clear(void);
u32 kernelmind_flow_count(void);

#endif /* __KERNELMIND_FLOW_TABLE_H */
