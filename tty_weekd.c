/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <curl/curl.h>

#include "config.h"

#define MAX_PATH 1024
#define MAX_RES 256

struct curl_response {
	char *buf;
	size_t len;
	size_t max;
};

/* Helper functions */
static void print_debug(const char *msg);
static char *get_save_path(void);
static void save_time_left(const char *time_left);
static size_t curl_write_cb(void *contents, size_t size, size_t nmemb, void *userp);
static void fetch_api(char *response, size_t max_size);
static void end_all_graphical_sessions(void);
static void kill_itself(void);
static void run(void);

static void
print_debug(const char *msg)
{
	if (debug_mode) {
		printf("tty_weekd: %s\n", msg);
	}
}

static char *
get_save_path(void)
{
	static char path[MAX_PATH];
	if (strncmp(save_location, "~/", 2) == 0) {
		const char *home = getenv("HOME");
		if (home) {
			snprintf(path, sizeof(path), "%s/%s", home, save_location + 2);
		} else {
			snprintf(path, sizeof(path), "%s", save_location + 2);
		}
	} else {
		snprintf(path, sizeof(path), "%s", save_location);
	}
	return path;
}

static void
save_time_left(const char *time_left)
{
	char *path = get_save_path();
	FILE *fp = fopen(path, "w");
	if (fp) {
		fprintf(fp, "%s", time_left);
		fclose(fp);
	} else {
		print_debug("open save file failed");
	}
}

static size_t
curl_write_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct curl_response *mem = (struct curl_response *)userp;

	size_t remain = mem->max - mem->len - 1;
	size_t copylen = realsize < remain ? realsize : remain;

	memcpy(&(mem->buf[mem->len]), contents, copylen);
	mem->len += copylen;
	mem->buf[mem->len] = '\0';

	return realsize;
}

static void
fetch_api(char *response, size_t max_size)
{
	CURL *curl;
	CURLcode res;
	struct curl_response chunk;

	chunk.buf = response;
	chunk.len = 0;
	chunk.max = max_size;

	strncpy(response, "WAITING", max_size);

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, api_url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			print_debug("curl failed");
		} else {
			/* Strip trailing newlines/spaces */
			for (int i = chunk.len - 1; i >= 0 && (response[i] == '\n' || response[i] == '\r' || response[i] == ' '); i--) {
				response[i] = '\0';
			}
		}

		curl_easy_cleanup(curl);
	}
}

static void
end_all_graphical_sessions(void)
{
	FILE *fp = popen("ps -A -o pid= -o comm= -o args=", "r");
	if (!fp) {
		print_debug("ps failed");
		return;
	}

	char line[1024];
	size_t count = sizeof(kill_targets) / sizeof(kill_targets[0]);

	while (fgets(line, sizeof(line), fp)) {
		for (size_t i = 0; i < count; i++) {
			if (strstr(line, kill_targets[i])) {
				int pid;
				if (sscanf(line, "%d", &pid) == 1 && pid != getpid()) {
					if (debug_mode) {
						char msg[256];
						snprintf(msg, sizeof(msg), "kill %d (%s)", pid, kill_targets[i]);
						print_debug(msg);
					}
					kill(pid, SIGKILL);
				}
				break;
			}
		}
	}
	pclose(fp);
}

static void
kill_itself(void)
{
	print_debug("exit");
	exit(0);
}

static void
run(void)
{
	char time_left_str[MAX_RES];
	long time_left;

	fetch_api(time_left_str, sizeof(time_left_str));
	save_time_left(time_left_str);

	while (strcmp(time_left_str, "WAITING") == 0) {
		print_debug("wait");
		sleep(wait_interval);
		fetch_api(time_left_str, sizeof(time_left_str));
		save_time_left(time_left_str);
	}

	if (strcmp(time_left_str, "END!") == 0) {
		print_debug("end");
		return;
	}

	time_left = atol(time_left_str);

	while (time_left >= 0) {
		end_all_graphical_sessions();

		char buf[64];
		snprintf(buf, sizeof(buf), "%ld", time_left);
		save_time_left(buf);

		if (debug_mode) {
			char sleep_msg[128];
			snprintf(sleep_msg, sizeof(sleep_msg), "sleep %d", run_interval);
			print_debug(sleep_msg);
		}

		sleep(run_interval);
		time_left -= run_interval;
	}

	save_time_left("END!");
	kill_itself();
}

int
main(int argc, char *argv[])
{
	/* In a suckless manner, just run it. Backgrounding can be done via standard UNIX tools like '&' or a separate wrapper.
	   For now, we act as a foreground process unless told otherwise, keeping it simple. */

	/* curl global init is recommended */
	curl_global_init(CURL_GLOBAL_DEFAULT);

	run();

	curl_global_cleanup();
	return 0;
}
