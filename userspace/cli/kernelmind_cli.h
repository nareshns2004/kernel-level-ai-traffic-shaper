/* SPDX-License-Identifier: MIT */
#ifndef KERNELMIND_CLI_H
#define KERNELMIND_CLI_H

#include "../../ebpf/common.h"

#define KERNELMIND_NL_FAMILY 31

int cmd_flows(int argc, char **argv);
int cmd_stats(int argc, char **argv);
int cmd_model(int argc, char **argv);
int cmd_policy(int argc, char **argv);

int kernelmind_nl_request(int cmd, void *req, size_t req_len,
			  void *resp, size_t resp_len);
void print_usage(const char *prog);

#endif /* KERNELMIND_CLI_H */
