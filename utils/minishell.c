#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
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
#include <plugin.h>
#include <xserver.h>
#include <timer.h>
#include <picker.h>
#include <keymap.h>
#include <nemoxml.h>
#include <nemolist.h>
#include <nemolistener.h>
#include <nemolog.h>
#include <nemoitem.h>
#include <nemomisc.h>

#include <nemoenvs.h>
#include <nemomirror.h>
#include <nemobus.h>
#include <nemodb.h>
#include <nemojson.h>
#include <nemoitem.h>
#include <nemotoken.h>
#include <nemomisc.h>

struct minishell {
	struct nemocompz *compz;
	struct nemoshell *shell;
	struct nemoenvs *envs;
	struct nemobus *bus;
	struct wl_event_source *busfd;
};

static int minishell_dispatch_command(struct minishell *mini, struct itemone *one)
{
	const char *type = nemoitem_one_get_attr(one, "type");

	if (strcmp(type, "app") == 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");
		const char *states = nemoitem_one_get_attr(one, "states");

		nemoenvs_launch_app(mini->envs, path, args, states);
	} else if (strcmp(type, "xapp") == 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");
		const char *states = nemoitem_one_get_attr(one, "states");

		nemoenvs_launch_xapp(mini->envs, path, args, states);
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
	}

	return 0;
}

static int minishell_dispatch_db(struct minishell *mini, const char *dburi, const char *dbname, const char *configpath, const char *themepath)
{
	struct nemodb *db;
	struct json_object *jobj;

	db = nemodb_create(dburi);
	if (db == NULL)
		return -1;

	nemodb_use_collection(db, dbname, configpath);

	jobj = nemodb_load_json_object(db);
	if (jobj != NULL) {
		nemoenvs_set_json_config(mini->envs, jobj);

		json_object_put(jobj);
	}

	nemodb_use_collection(db, dbname, themepath);

	jobj = nemodb_load_json_object(db);
	if (jobj != NULL) {
		nemoenvs_set_json_theme(mini->envs, jobj);

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
		nemoenvs_set_json_config(mini->envs, jobj);

		json_object_put(jobj);
	}

	jobj = nemojson_object_create_file(themepath);
	if (jobj != NULL) {
		nemoenvs_set_json_theme(mini->envs, jobj);

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

	nemoenvs_alive_app(mini->envs, pid, timeout);
}

static void minishell_destroy_client(void *data, pid_t pid)
{
	struct minishell *mini = (struct minishell *)data;

	if (nemoenvs_respawn_app(mini->envs, pid) > 0) {
	} else if (nemoenvs_detach_client(mini->envs, pid) != 0) {
	} else if (nemoenvs_detach_xclient(mini->envs, pid) != 0) {
	}
}

static void minishell_update_client(void *data, struct shellbin *bin, struct clientstate *state)
{
	struct minishell *mini = (struct minishell *)data;
	struct nemoshell *shell = mini->shell;

	if (nemoitem_one_has_attr(state->one, "mirrorscreen") != 0) {
		struct shellscreen *screen;

		screen = nemoshell_get_fullscreen(shell, nemoitem_one_get_attr(state->one, "mirrorscreen"));
		if (screen != NULL && screen->dw != 0 && screen->dh != 0) {
			struct nemomirror *mirror;

			mirror = nemomirror_create(shell, screen->dx, screen->dy, screen->dw, screen->dh, "overlay");
			if (mirror != NULL) {
				nemoshell_kill_fullscreen_bin(shell, screen->target);

				nemomirror_set_view(mirror, nemoshell_bin_get_view(bin));
				nemomirror_check_screen(mirror, screen);
			}
		}
	}
}

static void minishell_update_layer(void *data, struct shellbin *bin, const char *type)
{
	struct minishell *mini = (struct minishell *)data;

	if (strcmp(type, "background") == 0) {
		nemoview_put_state(nemoshell_bin_get_view(bin), NEMOVIEW_CATCH_STATE);
	}
}

static void minishell_update_transform(void *data, struct shellbin *bin)
{
	struct minishell *mini = (struct minishell *)data;
}

static void minishell_enter_idle(void *data)
{
	struct minishell *mini = (struct minishell *)data;

	nemoenvs_terminate_clients(mini->envs);
	nemoenvs_terminate_xclients(mini->envs);

	nemoenvs_execute_screensavers(mini->envs);
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
		{ "debug",							no_argument,				NULL,		'g' },
		{ "help",								no_argument,				NULL,		'h' },
		{ 0 }
	};

	struct minishell *mini;
	struct nemoshell *shell;
	struct nemocompz *compz;
	char *dbname = NULL;
	char *configpath = NULL;
	char *themepath = NULL;
	char *rendernode = env_get_string("NEMOSHELL_RENDER_NODE", NULL);
	char *evdevopts = env_get_string("NEMOSHELL_EVDEV_OPTS", NULL);
	char *seat = env_get_string("NEMOSHELL_SEAT_NAME", "seat0");
	int xdisplay = env_get_integer("NEMOSHELL_XDISPLAY_NUMBER", 0);
	int tty = env_get_integer("NEMOSHELL_TTY", 0);
	int opt;

	while (opt = getopt_long(argc, argv, "r:e:x:d:c:t:gh", options, NULL)) {
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

	if (configpath == NULL || themepath == NULL)
		return 0;

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
	nemoshell_set_enter_idle(shell, minishell_enter_idle);
	nemoshell_set_userdata(shell, mini);

	mini->compz = compz;
	mini->shell = shell;
	mini->envs = nemoenvs_create(shell);

	nemocompz_add_key_binding(compz, KEY_F1, MODIFIER_CTRL, nemoenvs_handle_terminal_key, (void *)mini->envs);
	nemocompz_add_key_binding(compz, KEY_F2, MODIFIER_CTRL, nemoenvs_handle_touch_key, (void *)mini->envs);
	nemocompz_add_key_binding(compz, KEY_ESC, MODIFIER_CTRL, nemoenvs_handle_escape_key, (void *)mini->envs);
	nemocompz_add_button_binding(compz, BTN_LEFT, nemoenvs_handle_left_button, (void *)mini->envs);
	nemocompz_add_button_binding(compz, BTN_RIGHT, nemoenvs_handle_right_button, (void *)mini->envs);
	nemocompz_add_touch_binding(compz, nemoenvs_handle_touch_event, (void *)mini->envs);

	nemocompz_make_current(compz);

	if (dbname != NULL)
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

	nemoenvs_execute_backgrounds(mini->envs);
	nemoenvs_execute_daemons(mini->envs);

	nemocompz_run(compz);

	nemoenvs_destroy(mini->envs);

	nemoshell_destroy(shell);

	nemocompz_destroy(compz);

	free(mini);

	return 0;
}
