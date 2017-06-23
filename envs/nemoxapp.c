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

static int nemoenvs_execute_xapp(struct nemoenvs *envs, struct nemoxserver *xserver, const char *_path, const char *_args, const char *_envp, const char *_states);
static int nemoenvs_execute_xserver(struct nemoenvs *envs, int xdisplay, const char *rendernode);

static struct nemoxserver *nemoenvs_search_xserver_ready(struct nemoenvs *envs)
{
	struct nemoxserver *xserver;

	wl_list_for_each(xserver, &envs->xserver_list, link) {
		if (xserver->state == NEMOXSERVER_READY_STATE)
			return xserver;
	}

	return NULL;
}

static struct nemoxserver *nemoenvs_search_xserver_display(struct nemoenvs *envs, int xdisplay)
{
	struct nemoxserver *xserver;

	wl_list_for_each(xserver, &envs->xserver_list, link) {
		if (xserver->xdisplay == xdisplay)
			return xserver;
	}

	return NULL;
}

static void nemoenvs_handle_xserver_sigusr1(struct wl_listener *listener, void *data)
{
	struct nemoenvs *envs = (struct nemoenvs *)container_of(listener, struct nemoenvs, xserver_listener);
	struct nemoxserver *xserver;
	struct nemoxapp *xapp, *next;

	wl_list_remove(&envs->xserver_listener.link);
	wl_list_init(&envs->xserver_listener.link);

	envs->is_waiting_sigusr1 = 0;

	wl_list_for_each_safe(xapp, next, &envs->xapp_list, link) {
		if (xapp->xdisplay >= 0 && envs->is_waiting_sigusr1 == 0) {
			xserver = nemoenvs_search_xserver_display(envs, xapp->xdisplay);
			if (xserver == NULL)
				nemoenvs_execute_xserver(envs, xapp->xdisplay, xapp->rendernode);

			wl_list_remove(&xapp->link);

			if (xapp->rendernode != NULL)
				free(xapp->rendernode);

			free(xapp);
		} else if (xapp->path != NULL) {
			xserver = nemoenvs_search_xserver_ready(envs);
			if (xserver == NULL)
				break;

			nemoenvs_execute_xapp(envs, xserver, xapp->path, xapp->args, xapp->envp, xapp->states);

			wl_list_remove(&xapp->link);

			if (xapp->states != NULL)
				free(xapp->states);

			if (xapp->args != NULL)
				free(xapp->args);

			free(xapp->path);
			free(xapp);
		}
	}

	if (envs->is_waiting_sigusr1 == 0 && wl_list_empty(&envs->xapp_list) == 0)
		nemoenvs_execute_xserver(envs, ++envs->xdisplay, NULL);
}

static int nemoenvs_execute_xserver(struct nemoenvs *envs, int xdisplay, const char *rendernode)
{
	struct nemoxserver *xserver;
	const char *xpath = envs->xserver.path;
	const char *xnode = envs->xserver.node;
	char display[256];

	xserver = nemoxserver_create(envs->shell, xpath, xdisplay);
	if (xserver == NULL)
		return -1;

	if (rendernode != NULL)
		nemoxserver_set_rendernode(xserver, rendernode);
	else if (xnode != NULL)
		nemoxserver_set_rendernode(xserver, xnode);

	nemoxserver_execute(xserver);

	wl_list_insert(&envs->xserver_list, &xserver->link);

	envs->xserver_listener.notify = nemoenvs_handle_xserver_sigusr1;
	wl_signal_add(&xserver->sigusr1_signal, &envs->xserver_listener);

	envs->is_waiting_sigusr1 = 1;

	return 0;
}

static int nemoenvs_execute_xapp(struct nemoenvs *envs, struct nemoxserver *xserver, const char *_path, const char *_args, const char *_envp, const char *_states)
{
	struct nemotoken *args;
	struct nemotoken *envp;
	pid_t pid;
	int i;

	args = nemotoken_create(_path, strlen(_path));
	if (_args != NULL)
		nemotoken_append_format(args, ";%s", _args);
	nemotoken_divide(args, ';');
	nemotoken_update(args);

	envp = nemotoken_create_simple();
	nemotoken_set_maximum(envp, 1024);
	nemotoken_append_format(envp, "DISPLAY=:%d;", xserver->xdisplay);
	if (_envp != NULL)
		nemotoken_append_format(envp, "%s;", _envp);
	for (i = 0; environ[i] != NULL; i++)
		nemotoken_append_format(envp, "%s;", environ[i]);
	nemotoken_divide(envp, ';');
	nemotoken_update(envp);

	pid = os_file_execute(nemotoken_get_token(args, 0), nemotoken_get_tokens(args), nemotoken_get_tokens(envp));
	if (pid > 0) {
		nemoenvs_attach_xclient(envs, xserver, pid, _path);

		if (_states != NULL) {
			struct nemotoken *states;
			struct clientstate *state;

			states = nemotoken_create(_states, strlen(_states));
			nemotoken_divide(states, ';');
			nemotoken_update(states);

			state = nemoshell_create_client_state(envs->shell, pid);
			clientstate_set_attrs(state,
					nemotoken_get_tokens(states),
					nemotoken_get_count(states) / 2);

			nemotoken_destroy(states);
		}
	}

	nemotoken_destroy(args);
	nemotoken_destroy(envp);

	return 0;
}

int nemoenvs_launch_xserver(struct nemoenvs *envs, int xdisplay, const char *rendernode)
{
	struct nemoxserver *xserver;
	struct nemoxapp *xapp;

	xserver = nemoenvs_search_xserver_display(envs, xdisplay);
	if (xserver != NULL)
		return 0;

	envs->xdisplay = xdisplay;

	if (envs->is_waiting_sigusr1 == 0) {
		nemoenvs_execute_xserver(envs, xdisplay, rendernode);

		return 0;
	}

	xapp = (struct nemoxapp *)malloc(sizeof(struct nemoxapp));
	xapp->path = NULL;
	xapp->args = NULL;
	xapp->states = NULL;
	xapp->xdisplay = xdisplay;
	xapp->rendernode = rendernode != NULL ? strdup(rendernode) : NULL;

	wl_list_insert(&envs->xapp_list, &xapp->link);

	return 0;
}

void nemoenvs_use_xserver(struct nemoenvs *envs, int xdisplay)
{
	env_set_format("DISPLAY", ":%d", xdisplay);
}

int nemoenvs_launch_xapp(struct nemoenvs *envs, const char *_path, const char *_args, const char *_envp, const char *_states)
{
	struct nemoxserver *xserver;
	struct nemoxapp *xapp;

	xserver = nemoenvs_search_xserver_ready(envs);
	if (xserver != NULL)
		return nemoenvs_execute_xapp(envs, xserver, _path, _args, _envp, _states);

	xapp = (struct nemoxapp *)malloc(sizeof(struct nemoxapp));
	xapp->path = strdup(_path);
	xapp->args = _args != NULL ? strdup(_args) : NULL;
	xapp->envp = _envp != NULL ? strdup(_envp) : NULL;
	xapp->states = _states != NULL ? strdup(_states) : NULL;
	xapp->xdisplay = -1;
	xapp->rendernode = NULL;

	wl_list_insert(&envs->xapp_list, &xapp->link);

	if (envs->is_waiting_sigusr1 == 0)
		nemoenvs_execute_xserver(envs, ++envs->xdisplay, NULL);

	return 0;
}

int nemoenvs_attach_xclient(struct nemoenvs *envs, struct nemoxserver *xserver, pid_t pid, const char *name)
{
	struct nemoxclient *xclient;

	xclient = (struct nemoxclient *)malloc(sizeof(struct nemoxclient));
	xclient->xserver = xserver;
	xclient->pid = pid;
	xclient->name = strdup(name);
	xclient->stime = time_current_msecs();

	xserver->state = NEMOXSERVER_USED_STATE;

	wl_list_insert(&envs->xclient_list, &xclient->link);

	nemolog_event("ENVS", "type(attach-xclient) name(%s) pid(%d)\n", name, pid);

	return 0;
}

int nemoenvs_detach_xclient(struct nemoenvs *envs, pid_t pid)
{
	struct nemoxclient *xclient;

	wl_list_for_each(xclient, &envs->xclient_list, link) {
		if (xclient->pid == pid) {
			wl_list_remove(&xclient->link);

			nemolog_event("ENVS", "type(detach-xclient) name(%s) pid(%d) runtime(%u)\n", xclient->name, xclient->pid, time_current_msecs() - xclient->stime);

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

			nemolog_event("ENVS", "type(terminate-xclient) name(%s) pid(%d) runtime(%u)\n", xclient->name, xclient->pid, time_current_msecs() - xclient->stime);

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

		if (xapp->rendernode != NULL)
			free(xapp->rendernode);
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
