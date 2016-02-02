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

	free(group->actions);
	free(group);
}

struct nemoenvs *nemoenvs_create(void)
{
	struct nemoenvs *envs;

	envs = (struct nemoenvs *)malloc(sizeof(struct nemoenvs));
	if (envs == NULL)
		return NULL;
	memset(envs, 0, sizeof(struct nemoenvs));

	envs->groups = (struct nemogroup **)malloc(sizeof(struct nemogroup *) * 8);
	envs->ngroups = 0;
	envs->sgroups = 8;

	return envs;
}

void nemoenvs_destroy(struct nemoenvs *envs)
{
	int i;

	for (i = 0; i < envs->ngroups; i++)
		nemoenvs_destroy_group(envs->groups[i]);

	free(envs->groups);
	free(envs);
}

void nemoenvs_load_background(struct nemoshell *shell)
{
	struct nemocompz *compz = shell->compz;
	int index;

	nemoitem_for_each(shell->configs, index, "//nemoshell/background", 0) {
		pid_t pid;
		char *argv[32];
		char *attr;
		int argc = 0;
		int iattr;
		int32_t x, y;
		int32_t width, height;

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
				clientstate_set_bin_flags(state, NEMO_SHELL_SURFACE_ALL_FLAGS);
			}
		}

		free(argv[1]);
		free(argv[2]);
		free(argv[3]);
		free(argv[4]);
	}
}

void nemoenvs_load_soundmanager(struct nemoshell *shell)
{
	int index;

	index = nemoitem_get(shell->configs, "//nemoshell/sound", 0);
	if (index >= 0) {
		char *path;
		char *attr;
		char *argv[32];
		int argc = 0;
		int i;

		argv[argc++] = nemoitem_get_attr(shell->configs, index, "path");

		nemoitem_for_vattr(shell->configs, index, "arg%d", i, 0, attr)
			argv[argc++] = attr;

		argv[argc++] = NULL;

		wayland_execute_path(argv[0], argv, NULL);
	}
}

void nemoenvs_load_actions(struct nemoshell *shell, struct nemoenvs *envs)
{
	struct nemogroup *group;
	struct nemoaction *action;
	char *path;
	char *icon;
	char *ring;
	char *attr;
	char *attr0, *attr1;
	int argc;
	int igroup;
	int index;
	int i;

	nemoitem_for_each(shell->configs, index, "//nemoshell/group", 0) {
		icon = nemoitem_get_attr(shell->configs, index, "icon");
		ring = nemoitem_get_attr(shell->configs, index, "ring");
		if (icon == NULL || ring == NULL)
			continue;

		group = nemoenvs_create_group();
		group->icon = strdup(icon);
		group->ring = strdup(ring);

		NEMOBOX_APPEND(envs->groups, envs->sgroups, envs->ngroups, group);
	}

	nemoitem_for_each(shell->configs, index, "//nemoshell/action", 0) {
		igroup = nemoitem_get_iattr(shell->configs, index, "group", 0);
		path = nemoitem_get_attr(shell->configs, index, "path");
		icon = nemoitem_get_attr(shell->configs, index, "icon");
		ring = nemoitem_get_attr(shell->configs, index, "ring");
		if (igroup < 0 || igroup >= envs->ngroups || icon == NULL || ring == NULL)
			continue;

		group = envs->groups[igroup];

		action = nemoenvs_create_action();
		action->icon = strdup(icon);
		action->ring = strdup(ring);

		action->type = NEMOENVS_ACTION_APP_TYPE;

		attr = nemoitem_get_attr(shell->configs, index, "type");
		if (attr != NULL) {
			if (strcmp(attr, "keypad") == 0)
				action->type = NEMOENVS_ACTION_KEYPAD_TYPE;
			else if (strcmp(attr, "speaker") == 0)
				action->type = NEMOENVS_ACTION_SPEAKER_TYPE;
		}

		action->input = NEMO_VIEW_INPUT_NORMAL;

		attr = nemoitem_get_attr(shell->configs, index, "input");
		if (attr != NULL) {
			if (strcmp(attr, "touch") == 0)
				action->input = NEMO_VIEW_INPUT_TOUCH;
		}

		action->flags = NEMO_SHELL_SURFACE_MOVABLE_FLAG | NEMO_SHELL_SURFACE_PICKABLE_FLAG | NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG | NEMO_SHELL_SURFACE_MINIMIZABLE_FLAG;

		attr = nemoitem_get_attr(shell->configs, index, "resize");
		if (attr != NULL) {
			if (strcmp(attr, "on") == 0)
				action->flags = NEMO_SHELL_SURFACE_MOVABLE_FLAG | NEMO_SHELL_SURFACE_RESIZABLE_FLAG | NEMO_SHELL_SURFACE_PICKABLE_FLAG | NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG | NEMO_SHELL_SURFACE_MINIMIZABLE_FLAG;
			else if (strcmp(attr, "scale") == 0)
				action->flags = NEMO_SHELL_SURFACE_MOVABLE_FLAG | NEMO_SHELL_SURFACE_SCALABLE_FLAG | NEMO_SHELL_SURFACE_PICKABLE_FLAG | NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG | NEMO_SHELL_SURFACE_MINIMIZABLE_FLAG;
		}

		attr0 = nemoitem_get_attr(shell->configs, index, "max_width");
		attr1 = nemoitem_get_attr(shell->configs, index, "max_height");
		if (attr0 != NULL && attr1 != NULL) {
			action->max_width = strtoul(attr0, NULL, 10);
			action->max_height = strtoul(attr1, NULL, 10);
			action->has_max_size = 1;
		} else {
			action->has_max_size = 0;
		}

		attr0 = nemoitem_get_attr(shell->configs, index, "min_width");
		attr1 = nemoitem_get_attr(shell->configs, index, "min_height");
		if (attr0 != NULL && attr1 != NULL) {
			action->min_width = strtoul(attr0, NULL, 10);
			action->min_height = strtoul(attr1, NULL, 10);
			action->has_min_size = 1;
		} else {
			action->has_min_size = 0;
		}

		attr = nemoitem_get_attr(shell->configs, index, "fadein_type");
		if (attr != NULL) {
			if (strcmp(attr, "alpha") == 0)
				action->fadein_type = NEMO_SHELL_FADEIN_ALPHA_FLAG;
			else if (strcmp(attr, "scale") == 0)
				action->fadein_type = NEMO_SHELL_FADEIN_SCALE_FLAG;
			else if (strcmp(attr, "alpha+scale") == 0)
				action->fadein_type = NEMO_SHELL_FADEIN_ALPHA_FLAG | NEMO_SHELL_FADEIN_SCALE_FLAG;

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
		}

		nemoitem_for_vattr(shell->configs, index, "arg%d", i, 0, attr)
			action->args[argc++] = attr;

		action->args[argc++] = NULL;

		NEMOBOX_APPEND(group->actions, group->sactions, group->nactions, action);
	}
}
