/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KERNELMIND_H
#define __KERNELMIND_H

#include <linux/netfilter.h>
#include <linux/skbuff.h>

unsigned int kernelmind_nf_hook(void *priv, struct sk_buff *skb,
				const struct nf_hook_state *state);

int kernelmind_proc_init(void);
void kernelmind_proc_exit(void);

int kernelmind_tc_init(const char *ifname);
void kernelmind_tc_exit(void);

struct kernelmind_global_stats;
struct kernelmind_global_stats *kernelmind_get_global_stats(void);

int kernelmind_model_import(void *data, size_t len);

#endif /* __KERNELMIND_H */
