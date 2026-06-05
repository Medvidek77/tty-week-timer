/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "config.h"

#define BUF_SIZE 4096

static int server_sock;

static void
handle_sigint(int sig)
{
	close(server_sock);
	exit(0);
}

static void
send_response(int client_sock, const char *body)
{
	char response[BUF_SIZE];
	int len = snprintf(response, sizeof(response),
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: %zu\r\n"
		"Connection: close\r\n\r\n"
		"%s", strlen(body), body);

	if (len > 0) {
		write(client_sock, response, len);
	}
}

static void
handle_client(int client_sock)
{
	char buffer[BUF_SIZE];
	ssize_t bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0';

		if (strncmp(buffer, "GET ", 4) == 0) {
			char state[256];
			FILE *fp = fopen(server_state_file, "r");
			if (fp) {
				if (fgets(state, sizeof(state), fp)) {
					size_t len = strcspn(state, "\r\n");
					state[len] = '\0';

					if (strcmp(state, "START") == 0)
						send_response(client_sock, "604800");
					else
						send_response(client_sock, state);
				} else {
					send_response(client_sock, "WAITING");
				}
				fclose(fp);
			} else {
				send_response(client_sock, "WAITING");
			}
		}
	}
	close(client_sock);
}

int
main(void)
{
	int client_sock;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);
	int opt = 1;

	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);

	/* Ignore SIGPIPE in case client drops connection early */
	signal(SIGPIPE, SIG_IGN);

	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_host);
	server_addr.sin_port = htons(server_port);

	if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(server_sock, 3) < 0) {
		perror("listen");
		exit(1);
	}

	if (debug_mode) {
		printf("listen: %s:%d\n", server_host, server_port);
		printf("state file: %s\n", server_state_file);
	}

	while (1) {
		client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
		if (client_sock < 0) {
			perror("accept");
			continue;
		}
		handle_client(client_sock);
	}

	return 0;
}
