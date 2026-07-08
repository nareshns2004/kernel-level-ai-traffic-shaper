/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "kmshaperd.h"

static int server_fd = -1;
static pthread_t server_thread;

static void *rest_server(void *arg)
{
	int port = *(int *)arg;
	struct sockaddr_in addr, client;
	socklen_t clen = sizeof(client);
	char req[1024];

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		return NULL;

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		goto out;

	if (listen(server_fd, 8) < 0)
		goto out;

	while (server_fd >= 0) {
		int fd = accept(server_fd, (struct sockaddr *)&client, &clen);
		char *metrics;
		char hdr[256];

		if (fd < 0)
			continue;

		read(fd, req, sizeof(req) - 1);

		metrics = metrics_render();
		snprintf(hdr, sizeof(hdr),
			 "HTTP/1.1 200 OK\r\n"
			 "Content-Type: text/plain; charset=utf-8\r\n"
			 "Content-Length: %zu\r\n"
			 "Connection: close\r\n\r\n",
			 metrics ? strlen(metrics) : 0);

		write(fd, hdr, strlen(hdr));
		if (metrics) {
			write(fd, metrics, strlen(metrics));
			free(metrics);
		}

		close(fd);
	}

out:
	if (server_fd >= 0) {
		close(server_fd);
		server_fd = -1;
	}
	return NULL;
}

int rest_api_start(int port)
{
	static int stored_port;

	stored_port = port;
	return pthread_create(&server_thread, NULL, rest_server, &stored_port);
}

void rest_api_stop(void)
{
	if (server_fd >= 0) {
		close(server_fd);
		server_fd = -1;
	}
	pthread_join(server_thread, NULL);
}
