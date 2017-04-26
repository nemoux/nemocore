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
#include <screen.h>
#include <view.h>
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

	nemolist_init(&envs->service_list);
	nemolist_init(&envs->client_list);

	wl_list_init(&envs->xserver_list);
	wl_list_init(&envs->xapp_list);
	wl_list_init(&envs->xclient_list);

	wl_list_init(&envs->xserver_listener.link);

	envs->legacy.pick_taps = 3;

	envs->terminal.path = env_get_string("NEMOSHELL_TERMINAL_PATH", "/usr/bin/weston-terminal");
	envs->terminal.args = env_get_string("NEMOSHELL_TERMINAL_ARGS", NULL);
	envs->xserver.path = env_get_string("NEMOSHELL_XSERVER_PATH", "/usr/bin/Xwayland");
	envs->xserver.node = env_get_string("NEMOSHELL_XSERVER_NODE", NULL);

	return envs;
}

void nemoenvs_destroy(struct nemoenvs *envs)
{
	wl_list_remove(&envs->xserver_listener.link);

	nemoitem_destroy(envs->apps);

	nemoenvs_terminate_xservers(envs);
	nemoenvs_terminate_xclients(envs);
	nemoenvs_terminate_xapps(envs);

	nemolist_remove(&envs->service_list);
	nemolist_remove(&envs->client_list);

	free(envs);
}
