#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoenvs.h>
#include <nemoxml.h>
#include <udphelper.h>
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
	if (action->text != NULL)
		free(action->text);
	if (action->type != NULL)
		free(action->type);
	if (action->keypad != NULL)
		free(action->keypad);
	if (action->layer != NULL)
		free(action->layer);
	if (action->network != NULL)
		free(action->network);
	if (action->resize != NULL)
		free(action->resize);

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

	if (group->icon != NULL)
		free(group->icon);
	if (group->ring != NULL)
		free(group->ring);
	if (group->type != NULL)
		free(group->type);
	if (group->path != NULL)
		free(group->path);

	free(group->actions);
	free(group);
}

struct nemoenvs *nemoenvs_create(struct nemotool *tool)
{
	struct nemoenvs *envs;

	envs = (struct nemoenvs *)malloc(sizeof(struct nemoenvs));
	if (envs == NULL)
		return NULL;
	memset(envs, 0, sizeof(struct nemoenvs));

	envs->configs = nemoitem_create(64);
	if (envs->configs == NULL)
		goto err1;

	envs->tool = tool;

	envs->groups = (struct nemogroup **)malloc(sizeof(struct nemogroup *) * 8);
	envs->ngroups = 0;
	envs->sgroups = 8;

	return envs;

err1:
	free(envs);

	return NULL;
}

void nemoenvs_destroy(struct nemoenvs *envs)
{
	int i;

	for (i = 0; i < envs->ngroups; i++)
		nemoenvs_destroy_group(envs->groups[i]);

	nemoitem_destroy(envs->configs);

	free(envs->groups);
	free(envs);
}

static int nemoenvs_dispatch_message(void *data)
{
	struct nemomsg *msg = (struct nemomsg *)data;
	int soc = nemomsg_get_socket(msg);
	char buffer[1024];
	char ip[64];
	int port;
	int size;

	size = udp_recv_from(soc, ip, &port, buffer, sizeof(buffer) - 1);
	if (size <= 0)
		return -1;

	return 0;
}

int nemoenvs_connect(struct nemoenvs *envs, const char *ip, int port)
{
	envs->msg = nemomsg_create(NULL, 0);
	if (envs->msg == NULL)
		return -1;
	nemomsg_set_client(envs->msg, "/shell", ip, port);
	nemomsg_send_message(envs->msg, "/shell", "merong", 6);

	envs->monitor = nemomonitor_create(envs->tool,
			nemomsg_get_socket(envs->msg),
			nemoenvs_dispatch_message,
			envs->msg);

	return 0;
}

void nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath)
{
	struct nemoxml *xml;
	struct xmlnode *node;
	int i, j;

	xml = nemoxml_create();
	nemoxml_load_file(xml, configpath);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->nodes, nodelink) {
		i = nemoitem_set(envs->configs, node->path);

		for (j = 0; j < node->nattrs; j++) {
			nemoitem_set_attr(envs->configs, i,
					node->attrs[j*2+0],
					node->attrs[j*2+1]);
		}
	}

	nemoxml_destroy(xml);
}

void nemoenvs_load_actions(struct nemoenvs *envs)
{
	struct nemogroup *group;
	struct nemoaction *action;
	char *path;
	char *icon;
	char *ring;
	char *text;
	char *attr;
	int igroup;
	int index;

	nemoitem_for_each(envs->configs, index, "//nemoshell/group", 0) {
		icon = nemoitem_get_attr(envs->configs, index, "icon");
		ring = nemoitem_get_attr(envs->configs, index, "ring");
		if (icon == NULL)
			continue;

		group = nemoenvs_create_group();
		group->icon = strdup(icon);

		if (ring != NULL)
			group->ring = strdup(ring);

		attr = nemoitem_get_attr(envs->configs, index, "type");
		if (attr != NULL)
			group->type = strdup(attr);

		attr = nemoitem_get_attr(envs->configs, index, "path");
		if (attr != NULL)
			group->path = strdup(attr);

		NEMOBOX_APPEND(envs->groups, envs->sgroups, envs->ngroups, group);
	}

	nemoitem_for_each(envs->configs, index, "//nemoshell/action", 0) {
		igroup = nemoitem_get_iattr(envs->configs, index, "group", 0);
		path = nemoitem_get_attr(envs->configs, index, "path");
		icon = nemoitem_get_attr(envs->configs, index, "icon");
		ring = nemoitem_get_attr(envs->configs, index, "ring");
		text = nemoitem_get_attr(envs->configs, index, "text");
		if (igroup < 0 || igroup >= envs->ngroups || icon == NULL)
			continue;

		group = envs->groups[igroup];

		action = nemoenvs_create_action();
		action->icon = strdup(icon);

		if (ring != NULL)
			action->ring = strdup(ring);

		if (text != NULL)
			action->text = strdup(text);

		attr = nemoitem_get_attr(envs->configs, index, "type");
		if (attr != NULL)
			action->type = strdup(attr);

		attr = nemoitem_get_attr(envs->configs, index, "keypad");
		if (attr != NULL)
			action->keypad = strdup(attr);

		attr = nemoitem_get_attr(envs->configs, index, "layer");
		if (attr != NULL)
			action->layer = strdup(attr);

		attr = nemoitem_get_attr(envs->configs, index, "network");
		if (attr != NULL)
			action->network = strdup(attr);

		attr = nemoitem_get_attr(envs->configs, index, "resize");
		if (attr != NULL)
			action->resize = strdup(attr);

		NEMOBOX_APPEND(group->actions, group->sactions, group->nactions, action);
	}
}
