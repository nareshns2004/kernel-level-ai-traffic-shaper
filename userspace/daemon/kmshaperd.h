/* SPDX-License-Identifier: MIT */
#ifndef KMSHAPERD_H
#define KMSHAPERD_H

#include "../../ebpf/common.h"

#define KMSHAPERD_DEFAULT_PORT 9102

void metrics_init(void);
void metrics_update(const struct kernelmind_global_stats *stats);
char *metrics_render(void);

int rest_api_start(int port);
void rest_api_stop(void);

#endif /* KMSHAPERD_H */
