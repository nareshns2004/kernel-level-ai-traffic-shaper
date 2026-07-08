/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "kmshaperd.h"

static volatile sig_atomic_t running = 1;

static void handle_signal(int sig)
{
	(void)sig;
	running = 0;
}

static int query_stats(struct kernelmind_global_stats *stats)
{
	struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
	struct nlmsghdr *nlh;
	struct iovec iov;
	struct msghdr msg = {};
	char buf[4096];
	int sock;

	sock = socket(AF_NETLINK, SOCK_RAW, KERNELMIND_NETLINK_FAMILY);
	if (sock < 0)
		return -1;

	memset(buf, 0, sizeof(buf));
	nlh = (struct nlmsghdr *)buf;
	nlh->nlmsg_len = NLMSG_LENGTH(0);
	nlh->nlmsg_type = 1;
	nlh->nlmsg_flags = NLM_F_REQUEST;
	nlh->nlmsg_seq = 1;
	nlh->nlmsg_pid = getpid();

	iov.iov_base = buf;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = &addr;
	msg.msg_namelen = sizeof(addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (sendmsg(sock, &msg, 0) < 0) {
		close(sock);
		return -1;
	}

	if (recv(sock, buf, sizeof(buf), 0) > 0)
		memcpy(stats, NLMSG_DATA(nlh), sizeof(*stats));

	close(sock);
	return 0;
}

int main(int argc, char **argv)
{
	int port = KMSHAPERD_DEFAULT_PORT;
	struct kernelmind_global_stats stats;

	if (argc > 1)
		port = atoi(argv[1]);

	signal(SIGTERM, handle_signal);
	signal(SIGHUP, handle_signal);
	signal(SIGINT, handle_signal);

	metrics_init();
	if (rest_api_start(port) < 0) {
		fprintf(stderr, "kmshaperd: failed to start REST API on %d\n",
			port);
		return 1;
	}

	printf("kmshaperd: listening on port %d\n", port);

	while (running) {
		if (query_stats(&stats) == 0)
			metrics_update(&stats);
		sleep(5);
	}

	rest_api_stop();
	printf("kmshaperd: stopped\n");
	return 0;
}
