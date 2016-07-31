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
#include <xserver.h>
#include <waylandhelper.h>

#include <nemoenvs.h>
#include <nemoxapp.h>
#include <nemotoken.h>
#include <nemolog.h>
#include <nemomisc.h>

int nemoenvs_launch_xserver0(struct nemoenvs *envs)
{
	struct nemoxserver *xserver;
	char display[256];

	xserver = nemoxserver_create(envs->shell,
			nemoitem_get_sattr(envs->configs, "/nemoshell/xserver", "path", NULL), 0);
	if (xserver == NULL)
		return -1;

	snprintf(display, sizeof(display), ":%d", 0);
	setenv("DISPLAY", display, 1);

	nemoxserver_execute(xserver);

	return 0;
}

static struct nemoxserver *nemoenvs_search_xserver(struct nemoenvs *envs)
{
	struct nemoxserver *xserver;

	wl_list_for_each(xserver, &envs->xserver_list, link) {
		if (xserver->state == NEMOXSERVER_READY_STATE)
			return xserver;
	}

	return NULL;
}

static int nemoenvs_execute_xapp(struct nemoenvs *envs, struct nemoxserver *xserver, const char *_path, const char *_args, struct clientstate *state);
static int nemoenvs_launch_xserver(struct nemoenvs *envs, uint32_t xdisplay);

static void nemoenvs_handle_xserver_sigusr1(struct wl_listener *listener, void *data)
{
	struct nemoenvs *envs = (struct nemoenvs *)container_of(listener, struct nemoenvs, xserver_listener);
	struct nemoxserver *xserver;
	struct nemoxapp *xapp, *next;

	wl_list_remove(&envs->xserver_listener.link);
	wl_list_init(&envs->xserver_listener.link);

	envs->is_waiting_sigusr1 = 0;

	wl_list_for_each_safe(xapp, next, &envs->xapp_list, link) {
		xserver = nemoenvs_search_xserver(envs);
		if (xserver == NULL)
			break;

		nemoenvs_execute_xapp(envs, xserver, xapp->path, xapp->args, xapp->state);

		wl_list_remove(&xapp->link);

		if (xapp->path != NULL)
			free(xapp->path);
		if (xapp->args != NULL)
			free(xapp->args);

		free(xapp);
	}

	if (wl_list_empty(&envs->xapp_list) == 0)
		nemoenvs_launch_xserver(envs, ++envs->xdisplay);
}

static int nemoenvs_launch_xserver(struct nemoenvs *envs, uint32_t xdisplay)
{
	if (envs->is_waiting_sigusr1 == 0) {
		struct nemoxserver *xserver;
		char display[256];

		xserver = nemoxserver_create(envs->shell,
				nemoitem_get_sattr(envs->configs, "/nemoshell/xserver", "path", NULL), xdisplay);
		if (xserver == NULL)
			return -1;

		nemoxserver_execute(xserver);

		wl_list_insert(&envs->xserver_list, &xserver->link);

		envs->xserver_listener.notify = nemoenvs_handle_xserver_sigusr1;
		wl_signal_add(&xserver->sigusr1_signal, &envs->xserver_listener);

		envs->is_waiting_sigusr1 = 1;
	}

	return 0;
}

static int nemoenvs_execute_xapp(struct nemoenvs *envs, struct nemoxserver *xserver, const char *_path, const char *_args, struct clientstate *state)
{
	struct nemotoken *args;
	struct nemotoken *envp;
	pid_t pid;

	args = nemotoken_create(_path, strlen(_path));
	if (_args != NULL)
		nemotoken_append_format(args, ";%s", _args);
	nemotoken_divide(args, ';');
	nemotoken_update(args);

	envp = nemotoken_create_format("DISPLAY=:%d;XDG_RUNTIME_DIR=/tmp", xserver->xdisplay);
	nemotoken_divide(envp, ';');
	nemotoken_update(envp);

	pid = wayland_execute_path(nemotoken_get_token(args, 0), nemotoken_get_tokens(args), nemotoken_get_tokens(envp));
	if (pid > 0) {
		nemoenvs_attach_xclient(envs, xserver, pid, _path);

		if (state != NULL)
			clientstate_set_pid(state, pid);
	}

	nemotoken_destroy(args);
	nemotoken_destroy(envp);

	return 0;
}

int nemoenvs_launch_xapp(struct nemoenvs *envs, const char *path, const char *args, struct clientstate *state)
{
	struct nemoxserver *xserver;
	struct nemoxapp *xapp;

	xserver = nemoenvs_search_xserver(envs);
	if (xserver != NULL)
		return nemoenvs_execute_xapp(envs, xserver, path, args, state);

	xapp = (struct nemoxapp *)malloc(sizeof(struct nemoxapp));
	xapp->path = strdup(path);
	xapp->args = args != NULL ? strdup(args) : NULL;
	xapp->state = state;

	wl_list_insert(&envs->xapp_list, &xapp->link);

	nemoenvs_launch_xserver(envs, ++envs->xdisplay);

	return 0;
}

int nemoenvs_attach_xclient(struct nemoenvs *envs, struct nemoxserver *xserver, pid_t pid, const char *name)
{
	struct nemoxclient *xclient;

	xclient = (struct nemoxclient *)malloc(sizeof(struct nemoxclient));
	xclient->xserver = xserver;
	xclient->pid = pid;
	xclient->name = strdup(name);

	xserver->state = NEMOXSERVER_USED_STATE;

	wl_list_insert(&envs->xclient_list, &xclient->link);

	return 0;
}

int nemoenvs_detach_xclient(struct nemoenvs *envs, pid_t pid)
{
	struct nemoxclient *xclient;

	wl_list_for_each(xclient, &envs->xclient_list, link) {
		if (xclient->pid == pid) {
			wl_list_remove(&xclient->link);

			xclient->xserver->state = NEMOXSERVER_READY_STATE;

			free(xclient->name);
			free(xclient);

			return 1;
		}
	}

	return 0;
}

int nemoenvs_terminate_xclient(struct nemoenvs *envs, pid_t pid)
{
	struct nemoxclient *xclient;

	wl_list_for_each(xclient, &envs->xclient_list, link) {
		if (xclient->pid == pid) {
			kill(-xclient->pid, SIGKILL);

			wl_list_remove(&xclient->link);

			xclient->xserver->state = NEMOXSERVER_READY_STATE;

			free(xclient->name);
			free(xclient);

			return 1;
		}
	}

	return 0;
}

int nemoenvs_terminate_xclients(struct nemoenvs *envs)
{
	struct nemoxclient *xclient;

	wl_list_for_each(xclient, &envs->xclient_list, link) {
		kill(-xclient->pid, SIGKILL);
	}

	return 0;
}

int nemoenvs_terminate_xservers(struct nemoenvs *envs)
{
	struct nemoxserver *xserver;

	wl_list_for_each(xserver, &envs->xserver_list, link) {
		kill(nemoxserver_get_pid(xserver), SIGKILL);
	}

	return 0;
}

int nemoenvs_terminate_xapps(struct nemoenvs *envs)
{
	struct nemoxapp *xapp, *next;

	wl_list_for_each_safe(xapp, next, &envs->xapp_list, link) {
		wl_list_remove(&xapp->link);

		if (xapp->path != NULL)
			free(xapp->path);
		if (xapp->args != NULL)
			free(xapp->args);

		free(xapp);
	}

	return 0;
}

int nemoenvs_get_xclient_count(struct nemoenvs *envs)
{
	return wl_list_length(&envs->xclient_list);
}
