#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
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
	struct itemattr *attr;
	char cmds[512];
	int32_t x, y;
	const char *path;
	const char *id;
	const char *name;
	const char *value;
	pid_t pid;

	x = nemoitem_one_get_iattr(one, "x", 0);
	y = nemoitem_one_get_iattr(one, "y", 0);
	path = nemoitem_one_get_attr(one, "path");
	id = nemoitem_one_get_attr(one, "id");

	strcpy(cmds, path);

	nemoitem_attr_for_each(attr, one) {
		name = nemoitem_attr_get_name(attr);
		value = nemoitem_attr_get_value(attr);

		strcat(cmds, ";--");
		strcat(cmds, name);
		strcat(cmds, ";");
		strcat(cmds, value);
	}

	token = nemotoken_create(cmds, strlen(cmds));
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

		nemoenvs_attach_app(envs, id, pid);
	}

	nemotoken_destroy(token);
}

static void nemoenvs_execute_daemon(struct nemoenvs *envs, struct itemone *one)
{
	struct nemotoken *token;
	struct itemattr *attr;
	char cmds[512];
	const char *path;
	const char *id;
	const char *name;
	const char *value;
	pid_t pid;

	path = nemoitem_one_get_attr(one, "path");
	id = nemoitem_one_get_attr(one, "id");

	strcpy(cmds, path);

	nemoitem_attr_for_each(attr, one) {
		name = nemoitem_attr_get_name(attr);
		value = nemoitem_attr_get_value(attr);

		strcat(cmds, ";--");
		strcat(cmds, name);
		strcat(cmds, ";");
		strcat(cmds, value);
	}

	token = nemotoken_create(cmds, strlen(cmds));
	nemotoken_divide(token, ';');
	nemotoken_update(token);

	pid = wayland_execute_path(path, nemotoken_get_tokens(token), NULL);
	if (pid > 0) {
		nemoenvs_attach_app(envs, id, pid);
	}

	nemotoken_destroy(token);
}

static void nemoenvs_execute_screensaver(struct nemoenvs *envs, struct itemone *one)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct nemotoken *token;
	struct itemattr *attr;
	char cmds[512];
	int32_t x, y;
	const char *path;
	const char *id;
	const char *name;
	const char *value;
	pid_t pid;

	x = nemoitem_one_get_iattr(one, "x", 0);
	y = nemoitem_one_get_iattr(one, "y", 0);
	path = nemoitem_one_get_attr(one, "path");
	id = nemoitem_one_get_attr(one, "id");

	strcpy(cmds, path);

	nemoitem_attr_for_each(attr, one) {
		name = nemoitem_attr_get_name(attr);
		value = nemoitem_attr_get_value(attr);

		strcat(cmds, ";--");
		strcat(cmds, name);
		strcat(cmds, ";");
		strcat(cmds, value);
	}

	token = nemotoken_create(cmds, strlen(cmds));
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
				} else if (nemoitem_one_has_path(one, "/nemoshell/daemon") != 0) {
					nemoenvs_execute_daemon(envs, one);
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

void nemoenvs_execute_daemons(struct nemoenvs *envs)
{
	struct itemone *one;

	nemoitem_for_each(one, envs->configs) {
		if (nemoitem_one_has_path(one, "/nemoshell/daemon") != 0) {
			nemoenvs_execute_daemon(envs, one);
		}
	}
}

void nemoenvs_execute_screensavers(struct nemoenvs *envs)
{
	struct itemone *one;

	nemoitem_for_each(one, envs->configs) {
		if (nemoitem_one_has_path(one, "/nemoshell/screensaver") != 0) {
			nemoenvs_execute_screensaver(envs, one);
		}
	}
}

int nemoenvs_attach_client(struct nemoenvs *envs, pid_t pid)
{
	struct nemoclient *client;

	client = (struct nemoclient *)malloc(sizeof(struct nemoclient));
	if (client == NULL)
		return -1;

	client->pid = pid;

	nemolist_insert(&envs->client_list, &client->link);

	return 0;
}

void nemoenvs_detach_client(struct nemoenvs *envs, pid_t pid)
{
	struct nemoclient *client;

	nemolist_for_each(client, &envs->client_list, link) {
		if (client->pid == pid) {
			nemolist_remove(&client->link);

			free(client);

			break;
		}
	}
}

int nemoenvs_terminate_client(struct nemoenvs *envs, pid_t pid)
{
	struct nemoclient *client;

	nemolist_for_each(client, &envs->client_list, link) {
		if (client->pid == pid) {
			nemolist_remove(&client->link);

			free(client);

			return 1;
		}
	}

	return 0;
}

int nemoenvs_terminate_clients(struct nemoenvs *envs)
{
	struct nemoclient *client;

	nemolist_for_each(client, &envs->client_list, link) {
		kill(client->pid, SIGKILL);
	}

	return 0;
}
