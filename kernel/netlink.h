/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KERNELMIND_NETLINK_H
#define __KERNELMIND_NETLINK_H

#include "../ebpf/common.h"

#define KERNELMIND_NL_CMD_STATS		1
#define KERNELMIND_NL_CMD_FLOWS		2
#define KERNELMIND_NL_CMD_RECLASSIFY	3
#define KERNELMIND_NL_CMD_MODEL_IMPORT	4
#define KERNELMIND_NL_CMD_MODEL_EXPORT	5
#define KERNELMIND_NL_CMD_POLICY_ADD	6
#define KERNELMIND_NL_CMD_OVERRIDE	7

int kernelmind_netlink_init(void);
void kernelmind_netlink_exit(void);
void kernelmind_netlink_broadcast_stats(void);

#endif /* __KERNELMIND_NETLINK_H */
