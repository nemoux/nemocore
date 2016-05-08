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
#include <view.h>
#include <waylandhelper.h>

#include <nemoenvs.h>
#include <nemolist.h>
#include <nemoitem.h>
#include <nemoease.h>
#include <nemomisc.h>

struct nemoaction *nemoenvs_create_action(void)
{
	struct nemoaction *action;

	action = (struct nemoaction *)malloc(sizeof(struct nemoaction));
	if (action == NULL)
		return NULL;
	memset(action, 0, sizeof(struct nemoaction));

	return action;
}

void nemoenvs_destroy_action(struct nemoaction *action)
{
	if (action->path != NULL)
		free(action->path);
	if (action->icon != NULL)
		free(action->icon);
	if (action->ring != NULL)
		free(action->ring);
	if (action->user != NULL)
		free(action->user);

	free(action);
}

struct nemogroup *nemoenvs_create_group(void)
{
	struct nemogroup *group;

	group = (struct nemogroup *)malloc(sizeof(struct nemogroup));
	if (group == NULL)
		return NULL;
	memset(group, 0, sizeof(struct nemogroup));

	group->actions = (struct nemoaction **)malloc(sizeof(struct nemoaction *) * 8);
	group->nactions = 0;
	group->sactions = 8;

	return group;
}

void nemoenvs_destroy_group(struct nemogroup *group)
{
	int i;

	for (i = 0; i < group->nactions; i++)
		nemoenvs_destroy_action(group->actions[i]);

	if (group->path != NULL)
		free(group->path);
	if (group->icon != NULL)
		free(group->icon);
	if (group->ring != NULL)
		free(group->ring);

	free(group->actions);
	free(group);
}

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

	free(app);
}

struct nemoenvs *nemoenvs_create(struct nemoshell *shell)
{
	struct nemoenvs *envs;

	envs = (struct nemoenvs *)malloc(sizeof(struct nemoenvs));
	if (envs == NULL)
		return NULL;
	memset(envs, 0, sizeof(struct nemoenvs));

	envs->shell = shell;

	envs->groups = (struct nemogroup **)malloc(sizeof(struct nemogroup *) * 8);
	envs->ngroups = 0;
	envs->sgroups = 8;

	nemolist_init(&envs->app_list);

	return envs;
}

void nemoenvs_destroy(struct nemoenvs *envs)
{
	int i;

	for (i = 0; i < envs->ngroups; i++)
		nemoenvs_destroy_group(envs->groups[i]);

	nemolist_remove(&envs->app_list);

	free(envs->groups);
	free(envs);
}

int nemoenvs_attach_app(struct nemoenvs *envs, int type, int index, pid_t pid)
{
	struct nemoapp *app;

	app = nemoenvs_create_app();
	if (app == NULL)
		return -1;

	app->type = type;
	app->index = index;
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

int nemoenvs_respawn_app(struct nemoenvs *envs, pid_t pid)
{
	struct nemoapp *app, *napp;

	nemolist_for_each_safe(app, napp, &envs->app_list, link) {
		if (app->pid == pid) {
			if (app->type == NEMOENVS_APP_BACKGROUND_TYPE) {
				nemoenvs_execute_background(envs, app->index);
			} else if (app->type == NEMOENVS_APP_SOUNDMANAGER_TYPE) {
				nemoenvs_execute_soundmanager(envs);
			}

			nemoenvs_destroy_app(app);

			return 1;
		}
	}

	return 0;
}

void nemoenvs_execute_background(struct nemoenvs *envs, int index)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	pid_t pid;
	int32_t x, y;
	int32_t width, height;
	char *argv[32];
	char *attr;
	int argc = 0;
	int iattr;

	x = nemoitem_get_iattr(shell->configs, index, "x", 0);
	y = nemoitem_get_iattr(shell->configs, index, "y", 0);
	width = nemoitem_get_iattr(shell->configs, index, "width", nemocompz_get_scene_width(compz));
	height = nemoitem_get_iattr(shell->configs, index, "height", nemocompz_get_scene_height(compz));

	argv[argc++] = nemoitem_get_attr(shell->configs, index, "path");
	argv[argc++] = strdup("-w");
	asprintf(&argv[argc++], "%d", width);
	argv[argc++] = strdup("-h");
	asprintf(&argv[argc++], "%d", height);

	nemoitem_for_vattr(shell->configs, index, "arg%d", iattr, 0, attr)
		argv[argc++] = attr;

	argv[argc++] = NULL;

	pid = wayland_execute_path(argv[0], argv, NULL);
	if (pid > 0) {
		struct clientstate *state;

		state = nemoshell_create_client_state(shell, pid);
		if (state != NULL) {
			clientstate_set_position(state, x, y);
			clientstate_set_anchor(state, 0.0f, 0.0f);
			clientstate_set_bin_flags(state, NEMOSHELL_SURFACE_ALL_FLAGS);
		}

		nemoenvs_attach_app(envs, NEMOENVS_APP_BACKGROUND_TYPE, index, pid);
	}

	free(argv[1]);
	free(argv[2]);
	free(argv[3]);
	free(argv[4]);
}

void nemoenvs_execute_backgrounds(struct nemoenvs *envs)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	int index;

	nemoitem_for_each(shell->configs, index, "//nemoshell/background", 0) {
		nemoenvs_execute_background(envs, index);
	}
}

void nemoenvs_execute_soundmanager(struct nemoenvs *envs)
{
	struct nemoshell *shell = envs->shell;
	int index;

	index = nemoitem_get(shell->configs, "//nemoshell/sound", 0);
	if (index >= 0) {
		pid_t pid;
		char *path;
		char *attr;
		char *argv[32];
		int argc = 0;
		int i;

		argv[argc++] = nemoitem_get_attr(shell->configs, index, "path");

		nemoitem_for_vattr(shell->configs, index, "arg%d", i, 0, attr)
			argv[argc++] = attr;

		argv[argc++] = NULL;

		pid = wayland_execute_path(argv[0], argv, NULL);
		if (pid > 0) {
			nemoenvs_attach_app(envs, NEMOENVS_APP_SOUNDMANAGER_TYPE, index, pid);
		}
	}
}

void nemoenvs_load_actions(struct nemoenvs *envs)
{
	struct nemoshell *shell = envs->shell;
	struct nemogroup *group;
	struct nemoaction *action;
	char *chromepath;
	char *path;
	char *icon;
	char *ring;
	char *attr;
	char *attr0, *attr1;
	int argc;
	int igroup;
	int index;
	int i;

	chromepath = nemoitem_get_attr_named(shell->configs, "//nemoshell/chrome", "path");

	nemoitem_for_each(shell->configs, index, "//nemoshell/group", 0) {
		icon = nemoitem_get_attr(shell->configs, index, "icon");
		ring = nemoitem_get_attr(shell->configs, index, "ring");
		if (icon == NULL)
			continue;

		group = nemoenvs_create_group();
		group->icon = strdup(icon);

		if (ring != NULL)
			group->ring = strdup(ring);

		group->type = NEMOENVS_GROUP_NORMAL_TYPE;

		attr = nemoitem_get_attr(shell->configs, index, "type");
		if (attr != NULL) {
			if (strcmp(attr, "contents") == 0) {
				path = nemoitem_get_attr(shell->configs, index, "path");
				if (path != NULL) {
					group->type = NEMOENVS_GROUP_CONTENTS_TYPE;
					group->path = strdup(path);
				}
			} else if (strcmp(attr, "bookmarks") == 0) {
				path = nemoitem_get_attr(shell->configs, index, "path");
				if (path != NULL) {
					group->type = NEMOENVS_GROUP_BOOKMARKS_TYPE;
					group->path = strdup(path);
				}
			}
		}

		NEMOBOX_APPEND(envs->groups, envs->sgroups, envs->ngroups, group);
	}

	nemoitem_for_each(shell->configs, index, "//nemoshell/action", 0) {
		igroup = nemoitem_get_iattr(shell->configs, index, "group", 0);
		path = nemoitem_get_attr(shell->configs, index, "path");
		icon = nemoitem_get_attr(shell->configs, index, "icon");
		ring = nemoitem_get_attr(shell->configs, index, "ring");
		if (igroup < 0 || igroup >= envs->ngroups || icon == NULL)
			continue;

		group = envs->groups[igroup];

		action = nemoenvs_create_action();
		action->icon = strdup(icon);

		if (ring != NULL)
			action->ring = strdup(ring);

		action->type = NEMOENVS_ACTION_APP_TYPE;
		action->keypad = 1;
		action->layer = 0;
		action->network = NEMOENVS_NETWORK_NORMAL_STATE;
		action->flags = NEMOSHELL_SURFACE_MOVABLE_FLAG | NEMOSHELL_SURFACE_PICKABLE_FLAG | NEMOSHELL_SURFACE_MAXIMIZABLE_FLAG | NEMOSHELL_SURFACE_MINIMIZABLE_FLAG;
		action->has_max_size = 0;
		action->has_min_size = 0;
		action->has_pickscreen = 0;
		action->has_pitchscreen = 0;

		attr = nemoitem_get_attr(shell->configs, index, "type");
		if (attr != NULL) {
			if (strcmp(attr, "chrome") == 0)
				action->type = NEMOENVS_ACTION_CHROME_TYPE;
			else if (strcmp(attr, "keypad") == 0)
				action->type = NEMOENVS_ACTION_KEYPAD_TYPE;
			else if (strcmp(attr, "speaker") == 0)
				action->type = NEMOENVS_ACTION_SPEAKER_TYPE;
			else if (strcmp(attr, "none") == 0)
				action->type = NEMOENVS_ACTION_NONE_TYPE;
		}

		attr = nemoitem_get_attr(shell->configs, index, "keypad");
		if (attr != NULL) {
			if (strcmp(attr, "off") == 0)
				action->keypad = 0;
		}

		attr = nemoitem_get_attr(shell->configs, index, "layer");
		if (attr != NULL) {
			if (strcmp(attr, "on") == 0)
				action->layer = 1;
		}

		attr = nemoitem_get_attr(shell->configs, index, "network");
		if (attr != NULL) {
			if (strcmp(attr, "block") == 0)
				action->network = NEMOENVS_NETWORK_BLOCK_STATE;
		}

		attr = nemoitem_get_attr(shell->configs, index, "resize");
		if (attr != NULL) {
			if (strcmp(attr, "on") == 0)
				action->flags = NEMOSHELL_SURFACE_MOVABLE_FLAG | NEMOSHELL_SURFACE_RESIZABLE_FLAG | NEMOSHELL_SURFACE_PICKABLE_FLAG | NEMOSHELL_SURFACE_MAXIMIZABLE_FLAG | NEMOSHELL_SURFACE_MINIMIZABLE_FLAG;
			else if (strcmp(attr, "scale") == 0)
				action->flags = NEMOSHELL_SURFACE_MOVABLE_FLAG | NEMOSHELL_SURFACE_SCALABLE_FLAG | NEMOSHELL_SURFACE_PICKABLE_FLAG | NEMOSHELL_SURFACE_MAXIMIZABLE_FLAG | NEMOSHELL_SURFACE_MINIMIZABLE_FLAG;
		}

		attr0 = nemoitem_get_attr(shell->configs, index, "max_width");
		attr1 = nemoitem_get_attr(shell->configs, index, "max_height");
		if (attr0 != NULL && attr1 != NULL) {
			action->max_width = strtoul(attr0, NULL, 10);
			action->max_height = strtoul(attr1, NULL, 10);
			action->has_max_size = 1;
		}

		attr0 = nemoitem_get_attr(shell->configs, index, "min_width");
		attr1 = nemoitem_get_attr(shell->configs, index, "min_height");
		if (attr0 != NULL && attr1 != NULL) {
			action->min_width = strtoul(attr0, NULL, 10);
			action->min_height = strtoul(attr1, NULL, 10);
			action->has_min_size = 1;
		}

		attr = nemoitem_get_attr(shell->configs, index, "pickscreen");
		if (attr != NULL && strcmp(attr, "on") == 0)
			action->has_pickscreen = 1;

		attr = nemoitem_get_attr(shell->configs, index, "pitchscreen");
		if (attr != NULL && strcmp(attr, "on") == 0)
			action->has_pitchscreen = 1;

		attr = nemoitem_get_attr(shell->configs, index, "fadein_type");
		if (attr != NULL) {
			if (strcmp(attr, "alpha") == 0)
				action->fadein_type = NEMOSHELL_FADEIN_ALPHA_FLAG;
			else if (strcmp(attr, "scale") == 0)
				action->fadein_type = NEMOSHELL_FADEIN_SCALE_FLAG;
			else if (strcmp(attr, "alpha+scale") == 0)
				action->fadein_type = NEMOSHELL_FADEIN_ALPHA_FLAG | NEMOSHELL_FADEIN_SCALE_FLAG;

			action->fadein_ease = NEMOEASE_CUBIC_OUT_TYPE;
			action->fadein_delay = 0;
			action->fadein_duration = 1200;

			attr = nemoitem_get_attr(shell->configs, index, "fadein_ease");
			if (attr != NULL)
				action->fadein_ease = nemoease_get_type(attr);

			attr = nemoitem_get_attr(shell->configs, index, "fadein_delay");
			if (attr != NULL)
				action->fadein_delay = strtoul(attr, NULL, 10);

			attr = nemoitem_get_attr(shell->configs, index, "fadein_duration");
			if (attr != NULL)
				action->fadein_duration = strtoul(attr, NULL, 10);
		}

		argc = 0;

		if (action->type == NEMOENVS_ACTION_APP_TYPE) {
			action->path = strdup(path);
			action->args[argc++] = action->path;
		} else if (action->type == NEMOENVS_ACTION_CHROME_TYPE) {
			action->path = strdup(chromepath);
			action->args[argc++] = action->path;

			attr = nemoitem_get_attr(shell->configs, index, "user");
			if (attr != NULL) {
				asprintf(&action->user, "--user-data-dir=%s", attr);
				action->args[argc++] = action->user;
			}
		}

		nemoitem_for_vattr(shell->configs, index, "arg%d", i, 0, attr)
			action->args[argc++] = attr;

		action->args[argc++] = NULL;

		NEMOBOX_APPEND(group->actions, group->sactions, group->nactions, action);
	}
}
