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

static volatile int running = 1;

static void
handle_sigint(int sig)
{
	running = 0;
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

		/* Only respond to GET requests */
		if (strncmp(buffer, "GET ", 4) == 0) {
			char state[256];
			FILE *fp = fopen(server_state_file, "r");
			if (fp) {
				if (fgets(state, sizeof(state), fp) != NULL) {
					/* Strip newline */
					size_t len = strlen(state);
					while (len > 0 && (state[len - 1] == '\n' || state[len - 1] == '\r')) {
						state[len - 1] = '\0';
						len--;
					}

					/* If state is START, return 604800 (1 week),
					 * otherwise return the state directly (WAITING, END! or seconds) */
					if (strcmp(state, "START") == 0) {
						send_response(client_sock, "604800");
					} else {
						send_response(client_sock, state);
					}
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
main(int argc, char *argv[])
{
	int server_sock, client_sock;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);
	int opt = 1;

	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);

	/* Ignore SIGPIPE in case client drops connection early */
	signal(SIGPIPE, SIG_IGN);

	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_host);
	server_addr.sin_port = htons(server_port);

	if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_sock, 3) < 0) {
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	printf("tty_week_server listening on %s:%d\n", server_host, server_port);
	printf("State is controlled by '%s'. Put 'START' for 1 week, 'WAITING' or 'END!'.\n", server_state_file);

	while (running) {
		client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
		if (client_sock < 0) {
			if (running) {
				perror("Accept failed");
			}
			continue;
		}

		handle_client(client_sock);
	}

	close(server_sock);
	printf("\nServer shut down.\n");
	return 0;
}
