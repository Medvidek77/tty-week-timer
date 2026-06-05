/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#define MAX_PATH 1024
#define MAX_RES 256

/* Helper functions */
static char *get_save_path(void);
static void fetch_time_left(char *buffer, size_t size);
static void get_end_as_local_time(const char *time_left_str, char *buffer, size_t size);
static void run(void);

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
fetch_time_left(char *buffer, size_t size)
{
	/* Default starting value similar to python's TIME_LEFT if file is absent */
	long default_time = 60 * 60 * 24 * 7;
	snprintf(buffer, size, "%ld", default_time);

	char *path = get_save_path();
	FILE *fp = fopen(path, "r");
	if (fp) {
		if (fgets(buffer, size, fp) != NULL) {
			/* Strip trailing newline */
			size_t len = strlen(buffer);
			while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
				buffer[len - 1] = '\0';
				len--;
			}
		}
		fclose(fp);
	}
}

static void
get_end_as_local_time(const char *time_left_str, char *buffer, size_t size)
{
	if (strcmp(time_left_str, "WAITING") == 0) {
		snprintf(buffer, size, "Still waiting for the challenge to start");
	} else if (strcmp(time_left_str, "END!") == 0) {
		snprintf(buffer, size, "The challenge is already over");
	} else {
		long tl = atol(time_left_str);
		time_t current_time = time(NULL);
		time_t end_time = current_time + tl;
		struct tm *tm_info = localtime(&end_time);

		strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
	}
}

static void
run(void)
{
	char time_left_str[MAX_RES];
	char end_time_str[MAX_RES];
	long time_left;
	long max_time = 604800; /* 7 days in seconds */

	fetch_time_left(time_left_str, sizeof(time_left_str));
	get_end_as_local_time(time_left_str, end_time_str, sizeof(end_time_str));

	printf("The challenge will end at: %s\n", end_time_str);

	if (strcmp(time_left_str, "WAITING") == 0) {
		time_left = max_time;
	} else if (strcmp(time_left_str, "END!") == 0) {
		time_left = 0;
	} else {
		time_left = atol(time_left_str);
	}

	printf("Time left: %lds\n", time_left);
	printf("You already spent %lds in the TTY\n", max_time - time_left);
}

int
main(int argc, char *argv[])
{
	run();
	return 0;
}
