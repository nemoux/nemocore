#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <signal.h>
#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <view.h>
#include <monitor.h>
#include <xserver.h>
#include <waylandhelper.h>

#include <nemoenvs.h>
#include <nemoapps.h>
#include <nemolist.h>
#include <nemotoken.h>
#include <nemolog.h>
#include <nemomisc.h>

struct nemoenvs *nemoenvs_create(struct nemoshell *shell)
{
	struct nemoenvs *envs;

	envs = (struct nemoenvs *)malloc(sizeof(struct nemoenvs));
	if (envs == NULL)
		return NULL;
	memset(envs, 0, sizeof(struct nemoenvs));

	envs->shell = shell;
	envs->apps = nemoitem_create();

	nemolist_init(&envs->app_list);
	nemolist_init(&envs->client_list);

	wl_list_init(&envs->xserver_list);
	wl_list_init(&envs->xapp_list);
	wl_list_init(&envs->xclient_list);

	wl_list_init(&envs->xserver_listener.link);

	envs->legacy.pick_taps = 3;

	return envs;
}

void nemoenvs_destroy(struct nemoenvs *envs)
{
	wl_list_remove(&envs->xserver_listener.link);

	nemoitem_destroy(envs->apps);

	nemoenvs_terminate_xservers(envs);
	nemoenvs_terminate_xclients(envs);
	nemoenvs_terminate_xapps(envs);

	nemolist_remove(&envs->app_list);
	nemolist_remove(&envs->client_list);

	free(envs);
}

void nemoenvs_set_terminal_path(struct nemoenvs *envs, const char *path)
{
	if (envs->terminal.path != NULL)
		free(envs->terminal.path);

	if (path != NULL)
		envs->terminal.path = strdup(path);
	else
		envs->terminal.path = NULL;
}

void nemoenvs_set_terminal_args(struct nemoenvs *envs, const char *args)
{
	if (envs->terminal.args != NULL)
		free(envs->terminal.args);

	if (args != NULL)
		envs->terminal.args = strdup(args);
	else
		envs->terminal.args = NULL;
}

void nemoenvs_set_xserver_path(struct nemoenvs *envs, const char *path)
{
	if (envs->xserver.path != NULL)
		free(envs->xserver.path);

	if (path != NULL)
		envs->xserver.path = strdup(path);
	else
		envs->xserver.path = NULL;
}

void nemoenvs_set_xserver_node(struct nemoenvs *envs, const char *node)
{
	if (envs->xserver.node != NULL)
		free(envs->xserver.node);

	if (node != NULL)
		envs->xserver.node = strdup(node);
	else
		envs->xserver.node = NULL;
}

int nemoenvs_set_service(struct nemoenvs *envs, struct itemone *one)
{
	struct itemone *tone;

	tone = nemoitem_search_one(envs->apps, nemoitem_one_get_path(one));
	if (tone != NULL)
		return -1;

	nemoitem_attach_one(envs->apps, nemoitem_one_clone(one));

	return 0;
}

void nemoenvs_put_service(struct nemoenvs *envs, const char *path)
{
	struct itemone *one;

	one = nemoitem_search_one(envs->apps, path);
	if (one != NULL)
		nemoitem_one_destroy(one);
}
