#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>

#include <wayland-server.h>

#include <json.h>

#include <shell.h>
#include <compz.h>
#include <backend.h>
#include <screen.h>
#include <view.h>
#include <content.h>
#include <session.h>
#include <binding.h>
#include <touch.h>
#include <plugin.h>
#include <xserver.h>
#include <timer.h>
#include <picker.h>
#include <keymap.h>

#include <nemoenvs.h>
#include <nemobus.h>
#include <nemodb.h>
#include <nemojson.h>
#include <nemoxml.h>
#include <nemothread.h>
#include <nemochannel.h>
#include <nemoqueue.h>
#include <nemolist.h>
#include <nemolistener.h>
#include <nemolog.h>
#include <nemoitem.h>
#include <nemotoken.h>
#include <nemomisc.h>
#include <pixmanhelper.h>

typedef enum {
	MINISHELL_NORMAL_STATE = 0,
	MINISHELL_IDLE_STATE = 1,
	MINISHELL_LAST_STATE
} MiniShellState;

struct minishell {
	struct nemocompz *compz;
	struct nemoshell *shell;
	struct nemoenvs *envs;
	struct nemobus *bus;
	struct wl_event_source *busfd;

	int state;

	struct {
		struct nemothread *thread;
		struct nemochannel *channel;
		struct nemoqueue *queue;
		struct nemotimer *timer;

		char *path;
		uint32_t interval;
	} screenshot;

	struct {
		struct wl_event_source *timer;

		uint32_t timeout;
	} screensaver;
};

struct miniclient {
	struct wl_listener destroy_listener;
	struct wl_listener focus_listener;

	struct wl_event_source *timer;
	uint32_t timeout;

	struct minishell *mini;
	struct shellbin *bin;
};

static int minishell_dispatch_command(struct minishell *mini, struct itemone *one)
{
	const char *type = nemoitem_one_get_attr(one, "type");

	if (strcmp(type, "app") == 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");
		const char *envp = nemoitem_one_get_attr(one, "envp");
		const char *states = nemoitem_one_get_attr(one, "states");

		nemoenvs_launch_app(mini->envs, path, args, envp, states);
	} else if (strcmp(type, "xapp") == 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");
		const char *envp = nemoitem_one_get_attr(one, "envp");
		const char *states = nemoitem_one_get_attr(one, "states");

		nemoenvs_launch_xapp(mini->envs, path, args, envp, states);
	} else if (strcmp(type, "service") == 0) {
		const char *group = nemoitem_one_get_attr(one, "group");

		if (nemoitem_one_has_sattr(one, "request", "stop") != 0)
			nemoenvs_stop_services(mini->envs, group);
		else
			nemoenvs_start_services(mini->envs, group);
	} else if (strcmp(type, "close") == 0) {
		struct nemoshell *shell = mini->shell;
		struct shellbin *bin;
		const char *uuid;

		uuid = nemoitem_one_get_attr(one, "uuid");
		if (uuid != NULL) {
			bin = nemoshell_get_bin_by_uuid(shell, uuid);
			if (bin != NULL)
				nemoshell_send_bin_close(bin);
		}
	} else if (strcmp(type, "close_all") == 0) {
		nemoenvs_terminate_clients(mini->envs);
		nemoenvs_terminate_xclients(mini->envs);
	} else if (strcmp(type, "close_fullscreen") == 0) {
		nemoshell_kill_fullscreen_bin(mini->shell, nemoitem_one_get_iattr(one, "target", 0));
	}

	return 0;
}

static int minishell_handle_json_config(struct minishell *mini, struct json_object *jobj)
{
	struct nemojson *json;
	int i;

	json = nemojson_create();
	nemojson_iterate_object(json, jobj);

	for (i = 0; i < nemojson_get_count(json); i++) {
		struct json_object *iobj = nemojson_get_object(json, i);
		const char *ikey = nemojson_get_key(json, i);

		if (strcmp(ikey, "screenshot") == 0) {
			mini->screenshot.path = nemojson_object_dup_string(iobj, "path", NULL);
			mini->screenshot.interval = nemojson_object_get_integer(iobj, "interval", 60 * 1000);
		} else if (strcmp(ikey, "screensaver") == 0) {
			mini->screensaver.timeout = nemojson_object_get_integer(iobj, "timeout", 0);
		}
	}

	nemojson_destroy(json);

	return 0;
}

static int minishell_handle_json_theme(struct minishell *mini, struct json_object *jobj)
{
	return 0;
}

static int minishell_dispatch_db(struct minishell *mini, const char *dburi, const char *dbname, const char *configpath, const char *themepath)
{
	struct nemodb *db;
	struct json_object *jobj;
	struct json_object *tobj;
	struct json_object *cobj;

	db = nemodb_create(dburi);
	if (db == NULL)
		return -1;

	nemodb_use_collection(db, dbname, configpath);

	jobj = nemodb_load_json_object(db);
	if (jobj != NULL) {
		tobj = nemojson_object_get_object(jobj, "configs", NULL);
		if (tobj != NULL) {
			cobj = nemojson_search_attribute(tobj,
					"id",
					nemojson_object_get_string(jobj, "useDefaultId", NULL));
			if (cobj != NULL) {
				nemoenvs_handle_json_config(mini->envs, cobj);
				minishell_handle_json_config(mini, cobj);
			}
		}

		json_object_put(jobj);
	}

	nemodb_use_collection(db, dbname, themepath);

	jobj = nemodb_load_json_object(db);
	if (jobj != NULL) {
		tobj = nemojson_object_get_object(jobj, "configs", NULL);
		if (tobj != NULL) {
			cobj = nemojson_search_attribute(tobj,
					"id",
					nemojson_object_get_string(jobj, "useDefaultId", NULL));
			if (cobj != NULL) {
				nemoenvs_handle_json_theme(mini->envs, cobj);
				minishell_handle_json_theme(mini, cobj);
			}
		}

		json_object_put(jobj);
	}

	nemodb_destroy(db);

	return 0;
}

static int minishell_dispatch_file(struct minishell *mini, const char *configpath, const char *themepath)
{
	struct json_object *jobj;

	jobj = nemojson_object_create_file(configpath);
	if (jobj != NULL) {
		nemoenvs_handle_json_config(mini->envs, jobj);
		minishell_handle_json_config(mini, jobj);

		json_object_put(jobj);
	}

	jobj = nemojson_object_create_file(themepath);
	if (jobj != NULL) {
		nemoenvs_handle_json_theme(mini->envs, jobj);
		minishell_handle_json_theme(mini, jobj);

		json_object_put(jobj);
	}

	return 0;
}

static int minishell_dispatch_bus(int fd, uint32_t mask, void *data)
{
	struct minishell *mini = (struct minishell *)data;
	struct nemojson *json;
	struct nemoitem *msg;
	struct itemone *one;
	char buffer[4096];
	int length;
	int i;

	length = nemobus_recv(mini->bus, buffer, sizeof(buffer));
	if (length <= 0)
		return 1;

	json = nemojson_create_string(buffer, length);
	nemojson_update(json);

	for (i = 0; i < nemojson_get_count(json); i++) {
		msg = nemoitem_create();
		nemojson_object_load_item(nemojson_get_object(json, i), msg, "/nemoshell");

		nemoitem_for_each(one, msg) {
			if (nemoitem_one_has_path(one, "/nemoshell/command") != 0)
				minishell_dispatch_command(mini, one);
		}

		nemoitem_destroy(msg);
	}

	nemojson_destroy(json);

	return 1;
}

static void minishell_alive_client(void *data, pid_t pid, uint32_t timeout)
{
	struct minishell *mini = (struct minishell *)data;

	nemoenvs_alive_service(mini->envs, pid, timeout);
}

static void minishell_destroy_client(void *data, pid_t pid)
{
	struct minishell *mini = (struct minishell *)data;

	if (nemoenvs_respawn_service(mini->envs, pid) > 0) {
	} else if (nemoenvs_detach_client(mini->envs, pid) != 0) {
	} else if (nemoenvs_detach_xclient(mini->envs, pid) != 0) {
	}
}

static int minishell_dispatch_bin_timeout(void *data)
{
	struct miniclient *client = (struct miniclient *)data;

	nemoshell_kill_bin(client->bin);

	return 1;
}

static void minishell_handle_bin_destroy(struct wl_listener *listener, void *data)
{
	struct miniclient *client = (struct miniclient *)container_of(listener, struct miniclient, destroy_listener);

	wl_list_remove(&client->destroy_listener.link);

	wl_event_source_remove(client->timer);

	free(client);
}

static void minishell_handle_bin_focus(struct wl_listener *listener, void *data)
{
	struct miniclient *client = (struct miniclient *)container_of(listener, struct miniclient, focus_listener);

	wl_event_source_timer_update(client->timer, client->timeout);
}

static void minishell_update_client(void *data, struct shellbin *bin, struct clientstate *state)
{
	struct minishell *mini = (struct minishell *)data;
	uint32_t timeout = nemoitem_one_get_iattr(state->one, "timeout", 0);

	if (timeout > 0) {
		struct miniclient *client;

		client = (struct miniclient *)malloc(sizeof(struct miniclient));
		client->mini = mini;
		client->bin = bin;

		client->destroy_listener.notify = minishell_handle_bin_destroy;
		wl_signal_add(&bin->destroy_signal, &client->destroy_listener);

		client->focus_listener.notify = minishell_handle_bin_focus;
		wl_signal_add(&bin->focus_signal, &client->focus_listener);

		client->timer = wl_event_loop_add_timer(mini->compz->loop, minishell_dispatch_bin_timeout, client);
		client->timeout = timeout;

		wl_event_source_timer_update(client->timer, client->timeout);
	}
}

static void minishell_update_layer(void *data, struct shellbin *bin, const char *type)
{
	struct minishell *mini = (struct minishell *)data;
}

static void minishell_update_transform(void *data, struct shellbin *bin)
{
	struct minishell *mini = (struct minishell *)data;
}

static void minishell_update_focus(void *data, struct shellbin *bin)
{
	struct minishell *mini = (struct minishell *)data;

	if (mini->state == MINISHELL_NORMAL_STATE) {
		wl_event_source_timer_update(mini->screensaver.timer, mini->screensaver.timeout);
	} else if (mini->state == MINISHELL_IDLE_STATE) {
		nemoenvs_stop_services(mini->envs, "screensaver");

		mini->state = MINISHELL_NORMAL_STATE;
	}
}

static int minishell_dispatch_screensaver_timeout(void *data)
{
	struct minishell *mini = (struct minishell *)data;

	nemoenvs_terminate_clients(mini->envs);
	nemoenvs_terminate_xclients(mini->envs);

	nemoenvs_start_services(mini->envs, "screensaver");

	mini->state = MINISHELL_IDLE_STATE;
}

static int minishell_dispatch_screenshot_thread(void *data)
{
	struct minishell *mini = (struct minishell *)data;
	struct eventone *eone;
	uint64_t event;

	while ((event = nemochannel_read(mini->screenshot.channel)) > 0) {
		while ((eone = nemoqueue_dequeue_one(mini->screenshot.queue)) != NULL) {
			const char *pngpath = nemoqueue_one_get_string(eone, 0);
			pixman_image_t *image = (pixman_image_t *)nemoqueue_one_get_pointer(eone, 0);

			pixman_save_png_file(image, pngpath);
			pixman_image_unref(image);

			nemoqueue_one_destroy(eone);
		}
	}

	return 0;
}

static void minishell_dispatch_screenshot_timer(struct nemotimer *timer, void *data)
{
	struct minishell *mini = (struct minishell *)data;
	struct nemoshell *shell = mini->shell;
	struct nemocompz *compz = mini->compz;
	struct nemoenvs *envs = mini->envs;
	struct nemoscreen *screen;

	wl_list_for_each(screen, &compz->screen_list, link) {
		struct eventone *eone;
		pixman_image_t *image;
		pixman_transform_t transform;
		char pngpath[128];
		char times[128];

		image = pixman_image_create_bits(PIXMAN_a8r8g8b8, screen->width, screen->height, NULL, screen->width * 4);
		pixman_transform_init_identity(&transform);
		pixman_transform_translate(&transform, NULL,
				pixman_double_to_fixed(screen->width / -2.0f),
				pixman_double_to_fixed(screen->height / -2.0f));
		pixman_transform_scale(&transform, NULL,
				pixman_double_to_fixed(1.0f),
				pixman_double_to_fixed(-1.0f));
		pixman_transform_translate(&transform, NULL,
				pixman_double_to_fixed(screen->width / 2.0f),
				pixman_double_to_fixed(screen->height / 2.0f));
		pixman_image_set_transform(image, &transform);

		nemoscreen_read_pixels(screen, PIXMAN_a8r8g8b8,
				pixman_image_get_data(image),
				screen->x, screen->y,
				screen->width, screen->height);

		time_get_string("_%Y%m%d_%H%M%S", times, sizeof(times));
		snprintf(pngpath, sizeof(pngpath), "%s/screen-%d-%d%s.png", mini->screenshot.path, screen->node->nodeid, screen->screenid, times);

		eone = nemoqueue_one_create();
		nemoqueue_one_set_max_string(eone, 1);
		nemoqueue_one_set_max_pointer(eone, 1);
		nemoqueue_one_set_string(eone, 0, pngpath);
		nemoqueue_one_set_pointer(eone, 0, image);
		nemoqueue_enqueue_one(mini->screenshot.queue, eone);
	}

	nemochannel_write(mini->screenshot.channel, 1);

	nemotimer_set_timeout(mini->screenshot.timer, mini->screenshot.interval);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "rendernode",					required_argument,	NULL,		'r' },
		{ "evdevopts",					required_argument,	NULL,		'e' },
		{ "xdisplay",						required_argument,	NULL,		'x' },
		{ "db",									required_argument,	NULL,		'd' },
		{ "config",							required_argument,	NULL,		'c' },
		{ "theme",							required_argument,	NULL,		't' },
		{ "service",						required_argument,	NULL,		's' },
		{ "framecheck",					required_argument,	NULL,		'f' },
		{ "debug",							no_argument,				NULL,		'g' },
		{ "help",								no_argument,				NULL,		'h' },
		{ 0 }
	};

	struct minishell *mini;
	struct nemoshell *shell;
	struct nemocompz *compz;
	char *dbname = "nemodb";
	char *configpath = "_shell_";
	char *themepath = "_theme_";
	char *service = NULL;
	char *rendernode = env_get_string("NEMOSHELL_RENDER_NODE", NULL);
	char *evdevopts = env_get_string("NEMOSHELL_EVDEV_OPTS", NULL);
	char *seat = env_get_string("NEMOSHELL_SEAT_NAME", "seat0");
	int xdisplay = env_get_integer("NEMOSHELL_XDISPLAY_NUMBER", 0);
	int tty = env_get_integer("NEMOSHELL_TTY", 0);
	int framecheck = env_get_integer("NEMOSHELL_FRAMECHECK", 0);
	int retval;
	int opt;

	while (opt = getopt_long(argc, argv, "r:e:x:d:c:t:s:f:gh", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'r':
				rendernode = strdup(optarg);
				break;

			case 'e':
				evdevopts = strdup(optarg);
				break;

			case 'x':
				xdisplay = strtoul(optarg, NULL, 10);
				break;

			case 'd':
				dbname = strdup(optarg);
				break;

			case 'c':
				configpath = strdup(optarg);
				break;

			case 't':
				themepath = strdup(optarg);
				break;

			case 's':
				service = strdup(optarg);
				break;

			case 'f':
				framecheck = strtoul(optarg, NULL, 10);
				break;

			case 'g':
				seat = NULL;
				break;

			case 'h':
				fprintf(stderr, "usage: minishell --db [dbname] --config [configname] --theme [themename]\n");
				return 0;

			default:
				break;
		}
	}

	mini = (struct minishell *)malloc(sizeof(struct minishell));
	if (mini == NULL)
		return -1;
	memset(mini, 0, sizeof(struct minishell));

	compz = nemocompz_create();
	nemocompz_load_backend(compz, "drm", rendernode);
	nemocompz_load_backend(compz, "evdev", evdevopts);
	nemocompz_connect_session(compz, seat, tty);

	shell = nemoshell_create(compz);
	nemoshell_set_alive_client(shell, minishell_alive_client);
	nemoshell_set_destroy_client(shell, minishell_destroy_client);
	nemoshell_set_update_client(shell, minishell_update_client);
	nemoshell_set_update_layer(shell, minishell_update_layer);
	nemoshell_set_update_transform(shell, minishell_update_transform);
	nemoshell_set_update_focus(shell, minishell_update_focus);
	nemoshell_set_frame_timeout(shell, framecheck);
	nemoshell_set_userdata(shell, mini);

	mini->compz = compz;
	mini->shell = shell;
	mini->envs = nemoenvs_create(shell);
	mini->state = MINISHELL_NORMAL_STATE;

	mini->screensaver.timer = wl_event_loop_add_timer(compz->loop, minishell_dispatch_screensaver_timeout, mini);
	mini->screensaver.timeout = 0;

	nemocompz_add_key_binding(compz, KEY_F1, MODIFIER_CTRL, nemoenvs_handle_terminal_key, (void *)mini->envs);
	nemocompz_add_key_binding(compz, KEY_F2, MODIFIER_CTRL, nemoenvs_handle_touch_key, (void *)mini->envs);
	nemocompz_add_key_binding(compz, KEY_ESC, MODIFIER_CTRL, nemoenvs_handle_escape_key, (void *)mini->envs);
	nemocompz_add_button_binding(compz, BTN_LEFT, nemoenvs_handle_left_button, (void *)mini->envs);
	nemocompz_add_button_binding(compz, BTN_RIGHT, nemoenvs_handle_right_button, (void *)mini->envs);
	nemocompz_add_touch_binding(compz, nemoenvs_handle_touch_event, (void *)mini->envs);

	nemocompz_make_current(compz);

	if (strstr(configpath, ".json") == NULL)
		minishell_dispatch_db(mini, "mongodb://127.0.0.1", dbname, configpath, themepath);
	else
		minishell_dispatch_file(mini, configpath, themepath);

	mini->bus = nemobus_create();
	nemobus_connect(mini->bus, NULL);
	nemobus_advertise(mini->bus, "set", "/nemoshell");

	mini->busfd = wl_event_loop_add_fd(
			nemocompz_get_wayland_event_loop(compz),
			nemobus_get_socket(mini->bus),
			WL_EVENT_READABLE,
			minishell_dispatch_bus,
			mini);

	nemoenvs_launch_xserver(mini->envs, xdisplay, rendernode);
	nemoenvs_use_xserver(mini->envs, xdisplay);

	if (service != NULL) {
		nemoenvs_start_services(mini->envs, service);
	} else {
		nemoenvs_start_services(mini->envs, "background");
		nemoenvs_start_services(mini->envs, "daemon");
	}

	if (mini->screenshot.path != NULL && mini->screenshot.interval > 0) {
		if (os_file_is_exist(mini->screenshot.path) == 0)
			mkdir(mini->screenshot.path, 0755);

		mini->screenshot.channel = nemochannel_create();
		nemochannel_set_blocking_mode(mini->screenshot.channel, 1);

		mini->screenshot.queue = nemoqueue_create();
		mini->screenshot.thread = nemothread_create(minishell_dispatch_screenshot_thread, mini);

		mini->screenshot.timer = nemotimer_create(compz);
		nemotimer_set_callback(mini->screenshot.timer, minishell_dispatch_screenshot_timer);
		nemotimer_set_userdata(mini->screenshot.timer, mini);
		nemotimer_set_timeout(mini->screenshot.timer, mini->screenshot.interval);
	}

	retval = nemocompz_run(compz);

	wl_event_source_remove(mini->screensaver.timer);

	nemoenvs_destroy(mini->envs);

	nemoshell_destroy(shell);

	nemocompz_destroy(compz);

	free(mini);

	return retval;
}
