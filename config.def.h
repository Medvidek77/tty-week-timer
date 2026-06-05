/* See LICENSE file for copyright and license details. */

/* API configuration */
static const char *api_url = "http://178.62.236.80/tty_week/api";

/* Save location for the timer. ~ is not expanded automatically in C,
 * so we will construct this dynamically using $HOME if not absolute.
 * If this path starts with "~/", it will be replaced by the user's home directory.
 */
static const char *save_location = "~/.tty_week_timer";

/* Daemon run interval in seconds.
 * Original config had RUN_EVERY = 10 (minutes) => 600 seconds.
 */
static const int run_interval = 600;

/* Wait interval in seconds if timer state is "WAITING".
 * Original was 10 seconds.
 */
static const int wait_interval = 10;

/* Debug mode flag (0 = off, 1 = on) */
static const int debug_mode = 1;

/* List of process names to identify and kill (X11 and Wayland compositors) */
static const char *kill_targets[] = {
	/* X11 targets */
	"Xorg",
	"X11",
	"xinit",
	"startx",
	"Xwayland",

	/* Wayland compositors / environments */
	"wayland",
	"sway",
	"Hyprland",
	"wlroots",
	"weston",
	"mutter",
	"kwin_wayland",
	"gnome-shell",
	"river",
	"dwl",
	"labwc",
	"hikari",
	"wayfire",
	"cage",
	"qtile",
	"wio"
};
