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

/* Helper functions */
static void print_debug(const char *msg);
static char *get_save_path(void);
static void save_time_left(const char *time_left);
static size_t curl_write_cb(void *contents, size_t size, size_t nmemb, void *userp);
static void fetch_api(char *response, size_t max_size);
static void kill_process_by_name(const char *name);
static void end_all_graphical_sessions(void);
static void kill_itself(void);
static void run(void);

static void
print_debug(const char *msg)
{
	if (debug_mode) {
		printf("[*] %s\n", msg);
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
	char debug_msg[MAX_RES + 32];
	snprintf(debug_msg, sizeof(debug_msg), "Saving time: %s", time_left);
	print_debug(debug_msg);

	char *path = get_save_path();
	FILE *fp = fopen(path, "w");
	if (fp) {
		fprintf(fp, "%s", time_left);
		fclose(fp);
	} else {
		print_debug("Failed to open save file for writing");
	}
}

static size_t
curl_write_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	char *response = (char *)userp;

	/* Copy up to MAX_RES-1 bytes */
	size_t copylen = realsize < (MAX_RES - 1) ? realsize : (MAX_RES - 1);
	memcpy(response, contents, copylen);
	response[copylen] = '\0';

	/* Remove trailing newlines/spaces */
	for (int i = copylen - 1; i >= 0 && (response[i] == '\n' || response[i] == '\r' || response[i] == ' '); i--) {
		response[i] = '\0';
	}

	return realsize;
}

static void
fetch_api(char *response, size_t max_size)
{
	CURL *curl;
	CURLcode res;

	print_debug("Initializing the timer by fetching it on the online API");

	/* Default to WAITING if fetch fails */
	strncpy(response, "WAITING", max_size);

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, api_url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
		/* Set a reasonable timeout */
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			char err_msg[256];
			snprintf(err_msg, sizeof(err_msg), "curl_easy_perform() failed: %s", curl_easy_strerror(res));
			print_debug(err_msg);
		} else {
			char debug_msg[MAX_RES + 64];
			snprintf(debug_msg, sizeof(debug_msg), "Found %s on the online API", response);
			print_debug(debug_msg);
		}

		curl_easy_cleanup(curl);
	}
}

static void
kill_process_by_name(const char *name)
{
	/* Use POSIX-compliant ps -A output */
	FILE *fp = popen("ps -A -o pid= -o comm= -o args=", "r");
	if (!fp) {
		print_debug("Failed to run ps");
		return;
	}

	char line[1024];
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strstr(line, name) != NULL) {
			int pid;
			if (sscanf(line, "%d", &pid) == 1) {
				/* Don't kill ourselves by accident if we match a substring */
				if (pid != getpid()) {
					char msg[256];
					snprintf(msg, sizeof(msg), "Found matching session '%s', killing pid %d", name, pid);
					print_debug(msg);
					kill(pid, SIGKILL);
				}
			}
		}
	}
	pclose(fp);
}

static void
end_all_graphical_sessions(void)
{
	size_t i;
	size_t count = sizeof(kill_targets) / sizeof(kill_targets[0]);

	for (i = 0; i < count; i++) {
		kill_process_by_name(kill_targets[i]);
	}
}

static void
kill_itself(void)
{
	print_debug("Killing myself");
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
		print_debug("Found waiting flag, will wait until something else happens");
		sleep(wait_interval);
		fetch_api(time_left_str, sizeof(time_left_str));
		save_time_left(time_left_str);
	}

	if (strcmp(time_left_str, "END!") == 0) {
		print_debug("Challenge already ended");
		return;
	}

	time_left = atol(time_left_str);

	while (time_left >= 0) {
		print_debug("Ending all graphical sessions");
		end_all_graphical_sessions();

		char buf[64];
		snprintf(buf, sizeof(buf), "%ld", time_left);
		save_time_left(buf);

		char sleep_msg[128];
		snprintf(sleep_msg, sizeof(sleep_msg), "Sleeping: %d", run_interval);
		print_debug(sleep_msg);

		sleep(run_interval);
		time_left -= run_interval;
	}

	print_debug("Challenge ended");
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
