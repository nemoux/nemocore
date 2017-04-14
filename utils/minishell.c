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

	struct {
		char *path;
		char *args;
	} keypad;

	struct {
		char *path;
		char *args;
	} content;

	struct {
		char *path;
		char *args;
	} chrome;
};

static void minishell_update_state(struct minishell *mini, struct itemone *one, struct clientstate *state)
{
	struct nemoview *view;
	const char *owner;

	clientstate_set_attrs(state, one);

	owner = nemoitem_one_get_attr(one, "owner");
	if (owner != NULL && (view = nemocompz_get_view_by_uuid(mini->compz, owner)) != NULL) {
		if (nemoitem_one_has_sattr(one, "coords", "global") == 0) {
			float x = nemoitem_one_get_fattr(one, "x", 0.0f);
			float y = nemoitem_one_get_fattr(one, "y", 0.0f);
			float r = nemoitem_one_get_fattr(one, "r", 0.0f) * M_PI / 180.0f;

			nemoview_transform_to_global(view, x, y, &x, &y);
			nemoview_rotate_to_global(view, r, &r);

			clientstate_set_fattr(state, "x", x);
			clientstate_set_fattr(state, "y", y);
			clientstate_set_fattr(state, "r", r * 180.0f / M_PI);
		}
	}
}

static int minishell_dispatch_keypad(struct minishell *mini, struct itemone *one)
{
	struct nemoshell *shell = mini->shell;
	struct nemocompz *compz = mini->compz;
	struct nemoview *view;
	struct nemoview *focus;
	struct nemotoken *args;
	const char *keypadpath = mini->keypad.path;
	const char *keypadargs = mini->keypad.args;
	float sx, sy;
	float x, y, r;
	pid_t pid;

	x = nemoitem_one_get_fattr(one, "x", 0.0f);
	y = nemoitem_one_get_fattr(one, "y", 0.0f);
	r = nemoitem_one_get_fattr(one, "r", 0.0f) * M_PI / 180.0f;

	focus = nemocompz_pick_view(compz, x, y, &sx, &sy, NEMOVIEW_PICK_STATE | NEMOVIEW_CANVAS_STATE);
	if (focus != NULL && nemoview_has_state(focus, NEMOVIEW_KEYPAD_STATE) != 0) {
		args = nemotoken_create(keypadpath, strlen(keypadpath));
		if (keypadargs != NULL) {
			nemotoken_append_one(args, ';');
			nemotoken_append_format(args, keypadargs, focus->uuid, nemoitem_one_get_sattr(one, "language", "eng"));
		}
		nemotoken_divide(args, ';');
		nemotoken_update(args);

		pid = os_execute_path(keypadpath, nemotoken_get_tokens(args), NULL);
		if (pid > 0) {
			struct clientstate *state;

			nemoenvs_attach_client(mini->envs, pid, keypadpath);

			state = nemoshell_create_client_state(shell, pid);
			if (state != NULL) {
				clientstate_set_fattr(state, "x", x);
				clientstate_set_fattr(state, "y", y);
				clientstate_set_fattr(state, "r", focus->geometry.r * 180.0f / M_PI);
			}
		}

		nemotoken_destroy(args);
	}

	return 0;
}

static int minishell_dispatch_xapp(struct minishell *mini, struct itemone *one)
{
	struct nemoshell *shell = mini->shell;
	struct clientstate *state;
	const char *_path = nemoitem_one_get_attr(one, "path");
	const char *_args = nemoitem_one_get_attr(one, "args");

	state = nemoshell_create_client_state(shell, 0);
	if (state != NULL)
		minishell_update_state(mini, one, state);

	nemoenvs_launch_xapp(mini->envs, _path, _args, state);

	return 0;
}

static int minishell_dispatch_app(struct minishell *mini, struct itemone *one)
{
	struct nemoshell *shell = mini->shell;
	struct clientstate *state;
	struct nemotoken *args;
	const char *_path = nemoitem_one_get_attr(one, "path");
	const char *_args = nemoitem_one_get_attr(one, "args");
	pid_t pid;

	args = nemotoken_create(_path, strlen(_path));
	if (_args != NULL)
		nemotoken_append_format(args, ";%s", _args);
	nemotoken_divide(args, ';');
	nemotoken_update(args);

	pid = os_execute_path(_path, nemotoken_get_tokens(args), NULL);
	if (pid > 0) {
		nemoenvs_attach_client(mini->envs, pid, _path);

		state = nemoshell_create_client_state(shell, pid);
		if (state != NULL)
			minishell_update_state(mini, one, state);
	}

	nemotoken_destroy(args);

	return 0;
}

static int minishell_dispatch_content(struct minishell *mini, struct itemone *one)
{
	struct nemotoken *args;
	struct clientstate *state;
	const char *_path = nemoitem_one_get_attr(one, "path");
	const char *contentpath = mini->content.path;
	const char *contentargs = mini->content.args;
	pid_t pid;

	args = nemotoken_create(contentpath, strlen(contentpath));
	if (contentargs != NULL) {
		nemotoken_append_one(args, ';');
		nemotoken_append_format(args, contentargs, _path);
	}
	nemotoken_divide(args, ';');
	nemotoken_update(args);

	pid = os_execute_path(contentpath, nemotoken_get_tokens(args), NULL);
	if (pid > 0) {
		nemoenvs_attach_client(mini->envs, pid, contentpath);

		state = nemoshell_create_client_state(mini->shell, pid);
		if (state != NULL)
			minishell_update_state(mini, one, state);
	}

	nemotoken_destroy(args);

	return 0;
}

static int minishell_dispatch_bookmark(struct minishell *mini, struct itemone *one)
{
	struct nemotoken *args;
	struct clientstate *state;
	const char *_path = nemoitem_one_get_attr(one, "path");
	const char *chromepath = mini->chrome.path;
	const char *chromeargs = mini->chrome.args;
	pid_t pid;

	args = nemotoken_create(chromepath, strlen(chromepath));
	if (chromeargs != NULL) {
		nemotoken_append_one(args, ';');
		nemotoken_append_format(args, chromeargs, _path);
	}
	nemotoken_divide(args, ';');
	nemotoken_update(args);

	pid = os_execute_path(chromepath, nemotoken_get_tokens(args), NULL);
	if (pid > 0) {
		nemoenvs_attach_client(mini->envs, pid, chromepath);

		state = nemoshell_create_client_state(mini->shell, pid);
		if (state != NULL)
			minishell_update_state(mini, one, state);
	}

	nemotoken_destroy(args);

	return 0;
}

static int minishell_dispatch_close(struct minishell *mini, struct itemone *one)
{
	struct nemoshell *shell = mini->shell;
	struct shellbin *bin;
	const char *uuid;

	uuid = nemoitem_one_get_attr(one, "uuid");
	if (uuid != NULL) {
		bin = nemoshell_get_bin_by_uuid(shell, uuid);
		if (bin != NULL)
			nemoshell_send_bin_close(bin);
	}

	return 0;
}

static int minishell_dispatch_close_all(struct minishell *mini, struct itemone *one)
{
	nemoenvs_terminate_clients(mini->envs);
	nemoenvs_terminate_xclients(mini->envs);

	return 0;
}

static int minishell_dispatch_command(struct minishell *mini, struct itemone *one)
{
	const char *type = nemoitem_one_get_attr(one, "type");

	if (strcmp(type, "keypad") == 0)
		minishell_dispatch_keypad(mini, one);
	else if (strcmp(type, "xapp") == 0)
		minishell_dispatch_xapp(mini, one);
	else if (strcmp(type, "app") == 0)
		minishell_dispatch_app(mini, one);
	else if (strcmp(type, "content") == 0)
		minishell_dispatch_content(mini, one);
	else if (strcmp(type, "bookmark") == 0)
		minishell_dispatch_bookmark(mini, one);
	else if (strcmp(type, "close") == 0)
		minishell_dispatch_close(mini, one);
	else if (strcmp(type, "close_all") == 0)
		minishell_dispatch_close_all(mini, one);

	return 0;
}

static int minishell_dispatch_config(struct minishell *mini, struct itemone *one)
{
	if (nemoitem_one_has_path_prefix(one, "/nemoshell/keypad") != 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");

		mini->keypad.path = path != NULL ? strdup(path) : NULL;
		mini->keypad.args = args != NULL ? strdup(args) : NULL;
	} else if (nemoitem_one_has_path_prefix(one, "/nemoshell/chrome") != 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");

		mini->chrome.path = path != NULL ? strdup(path) : NULL;
		mini->chrome.args = args != NULL ? strdup(args) : NULL;
	} else if (nemoitem_one_has_path_prefix(one, "/nemoshell/content") != 0) {
		const char *path = nemoitem_one_get_attr(one, "path");
		const char *args = nemoitem_one_get_attr(one, "args");

		mini->content.path = path != NULL ? strdup(path) : NULL;
		mini->content.args = args != NULL ? strdup(args) : NULL;
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
	}

	nemodb_destroy(db);

	return 0;
}

static int minishell_dispatch_file(struct minishell *mini, const char *configpath, const char *themepath)
{
	struct json_object *jobj;

	jobj = nemojson_object_create_file(configpath);
	if (jobj != NULL) {
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
