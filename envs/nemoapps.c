#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <waylandhelper.h>

#include <nemoenvs.h>
#include <nemoapps.h>
#include <nemolist.h>
#include <nemoitem.h>
#include <nemomisc.h>

struct nemoapp *nemoenvs_create_app(void)
{
	struct nemoapp *app;

	app = (struct nemoapp *)malloc(sizeof(struct nemoapp));
	if (app == NULL)
		return NULL;
	memset(app, 0, sizeof(struct nemoapp));

	nemolist_init(&app->link);

	return app;
}

void nemoenvs_destroy_app(struct nemoapp *app)
{
	nemolist_remove(&app->link);

	if (app->id != NULL)
		free(app->id);

	free(app);
}

int nemoenvs_attach_app(struct nemoenvs *envs, const char *id, pid_t pid)
{
	struct nemoapp *app;

	app = nemoenvs_create_app();
	if (app == NULL)
		return -1;

	app->id = strdup(id);
	app->pid = pid;

	nemolist_insert(&envs->app_list, &app->link);

	return 0;
}

void nemoenvs_detach_app(struct nemoenvs *envs, pid_t pid)
{
	struct nemoapp *app, *napp;

	nemolist_for_each_safe(app, napp, &envs->app_list, link) {
		if (app->pid == pid) {
			nemoenvs_destroy_app(app);
			return;
		}
	}
}

static void nemoenvs_execute_background(struct nemoenvs *envs, struct itemone *one)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct nemotoken *token;
	pid_t pid;
	int32_t x, y;
	int32_t width, height;
	const char *path;
	const char *args;

	x = nemoitem_one_get_iattr(one, "x", 0);
	y = nemoitem_one_get_iattr(one, "y", 0);
	width = nemoitem_one_get_iattr(one, "width", nemocompz_get_scene_width(compz));
	height = nemoitem_one_get_iattr(one, "height", nemocompz_get_scene_height(compz));
	path = nemoitem_one_get_attr(one, "path");
	args = nemoitem_one_get_attr(one, "args");

	token = nemotoken_create_format("%s;-w;%d;-h;%d", path, width, height);
	if (args != NULL)
		nemotoken_append_format(token, ";%s", args);
	nemotoken_divide(token, ';');
	nemotoken_update(token);

	pid = wayland_execute_path(path, nemotoken_get_tokens(token), NULL);
	if (pid > 0) {
		struct clientstate *state;

		state = nemoshell_create_client_state(shell, pid);
		if (state != NULL) {
			clientstate_set_position(state, x, y);
			clientstate_set_anchor(state, 0.0f, 0.0f);
			clientstate_set_bin_flags(state, NEMOSHELL_SURFACE_ALL_FLAGS);
		}

		nemoenvs_attach_app(envs,
				nemoitem_one_get_sattr(one, "id", ""),
				pid);
	}

	nemotoken_destroy(token);
}

static void nemoenvs_execute_soundmanager(struct nemoenvs *envs, struct itemone *one)
{
	struct nemotoken *token;
	pid_t pid;
	const char *path;
	const char *args;

	path = nemoitem_one_get_attr(one, "path");
	args = nemoitem_one_get_attr(one, "args");

	token = nemotoken_create(path, strlen(path));
	if (args != NULL)
		nemotoken_append_format(token, ";%s", args);
	nemotoken_divide(token, ';');
	nemotoken_update(token);

	pid = wayland_execute_path(path, nemotoken_get_tokens(token), NULL);
	if (pid > 0) {
		nemoenvs_attach_app(envs,
				nemoitem_one_get_sattr(one, "id", ""),
				pid);
	}

	nemotoken_destroy(token);
}

int nemoenvs_respawn_app(struct nemoenvs *envs, pid_t pid)
{
	struct nemoapp *app, *napp;
	struct itemone *one;

	nemolist_for_each_safe(app, napp, &envs->app_list, link) {
		if (app->pid == pid) {
			one = nemoitem_search_attr(envs->configs, NULL, "id", app->id);
			if (one != NULL) {
				if (nemoitem_one_has_path(one, "/nemoshell/background") != 0) {
					nemoenvs_execute_background(envs, one);
				} else if (nemoitem_one_has_path(one, "/nemoshell/sound") != 0) {
					nemoenvs_execute_soundmanager(envs, one);
				}
			}

			nemoenvs_destroy_app(app);

			return 1;
		}
	}

	return 0;
}

void nemoenvs_execute_backgrounds(struct nemoenvs *envs)
{
	struct itemone *one;

	nemoitem_for_each(one, envs->configs) {
		if (nemoitem_one_has_path(one, "/nemoshell/background") != 0) {
			nemoenvs_execute_background(envs, one);
		}
	}
}

void nemoenvs_execute_soundmanagers(struct nemoenvs *envs)
{
	struct itemone *one;

	nemoitem_for_each(one, envs->configs) {
		if (nemoitem_one_has_path(one, "/nemoshell/sound") != 0) {
			nemoenvs_execute_soundmanager(envs, one);
		}
	}
}
