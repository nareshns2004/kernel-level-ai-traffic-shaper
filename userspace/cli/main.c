/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "kernelmind_cli.h"

int kernelmind_nl_request(int cmd, void *req, size_t req_len,
			  void *resp, size_t resp_len)
{
	struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
	struct nlmsghdr *nlh;
	struct iovec iov;
	struct msghdr msg = {};
	char buf[4096];
	int sock;
	ssize_t ret;

	sock = socket(AF_NETLINK, SOCK_RAW, KERNELMIND_NL_FAMILY);
	if (sock < 0)
		return -errno;

	nlh = (struct nlmsghdr *)buf;
	memset(buf, 0, sizeof(buf));
	nlh->nlmsg_len = NLMSG_LENGTH(req_len);
	nlh->nlmsg_type = cmd;
	nlh->nlmsg_flags = NLM_F_REQUEST;
	nlh->nlmsg_seq = 1;
	nlh->nlmsg_pid = getpid();

	if (req_len && req)
		memcpy(NLMSG_DATA(nlh), req, req_len);

	iov.iov_base = buf;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = &addr;
	msg.msg_namelen = sizeof(addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (sendmsg(sock, &msg, 0) < 0) {
		ret = -errno;
		close(sock);
		return ret;
	}

	ret = recv(sock, buf, sizeof(buf), 0);
	if (ret > 0 && resp && resp_len)
		memcpy(resp, NLMSG_DATA(nlh), resp_len < (size_t)ret ?
		       resp_len : (size_t)ret);

	close(sock);
	return 0;
}

void print_usage(const char *prog)
{
	printf("Usage: %s <command> [options]\n\n", prog);
	printf("Commands:\n");
	printf("  flows              Show live flow table\n");
	printf("  stats              Show module statistics\n");
	printf("  reclassify         Force reclassify all flows\n");
	printf("  model export       Export model weights\n");
	printf("  model import       Import model weights\n");
	printf("  policy add         Add shaping policy\n");
	printf("  override           Manual flow override\n");
	printf("  help               Show this help\n");
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		print_usage(argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "flows") == 0)
		return cmd_flows(argc, argv);
	if (strcmp(argv[1], "stats") == 0)
		return cmd_stats(argc, argv);
	if (strcmp(argv[1], "reclassify") == 0)
		return kernelmind_nl_request(KERNELMIND_NL_CMD_RECLASSIFY,
					     NULL, 0, NULL, 0);
	if (strcmp(argv[1], "model") == 0)
		return cmd_model(argc, argv);
	if (strcmp(argv[1], "policy") == 0)
		return cmd_policy(argc, argv);
	if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0) {
		print_usage(argv[0]);
		return 0;
	}

	fprintf(stderr, "Unknown command: %s\n", argv[1]);
	print_usage(argv[0]);
	return 1;
}
