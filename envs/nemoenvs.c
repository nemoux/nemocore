#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <view.h>
#include <monitor.h>
#include <timer.h>
#include <waylandhelper.h>

#include <nemoenvs.h>
#include <nemoapps.h>
#include <nemolist.h>
#include <nemoitem.h>
#include <nemoease.h>
#include <nemotoken.h>
#include <udphelper.h>
#include <stringhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

#define NEMOENVS_LIVENESS_TIMEOUT		(7000)

struct nemoenvs *nemoenvs_create(struct nemoshell *shell)
{
	struct nemoenvs *envs;

	envs = (struct nemoenvs *)malloc(sizeof(struct nemoenvs));
	if (envs == NULL)
		return NULL;
	memset(envs, 0, sizeof(struct nemoenvs));

	envs->shell = shell;

	envs->configs = nemoitem_create();
	if (envs->configs == NULL)
		goto err1;

	nemolist_init(&envs->app_list);
	nemolist_init(&envs->callback_list);

	nemoenvs_set_callback(envs, nemoenvs_dispatch_message, shell);

	return envs;

err1:
	free(envs);

	return NULL;
}

void nemoenvs_destroy(struct nemoenvs *envs)
{
	struct envscallback *cb, *next;

	nemolist_for_each_safe(cb, next, &envs->callback_list, link) {
		nemolist_remove(&cb->link);

		free(cb);
	}

	nemolist_remove(&envs->app_list);

	nemoitem_destroy(envs->configs);

	if (envs->monitor != NULL)
		nemomonitor_destroy(envs->monitor);
	if (envs->msg != NULL)
		nemomsg_destroy(envs->msg);
	if (envs->timer != NULL)
		nemotimer_destroy(envs->timer);

	if (envs->name != NULL)
		free(envs->name);

	free(envs);
}

void nemoenvs_set_name(struct nemoenvs *envs, const char *name)
{
	if (envs->name != NULL)
		free(envs->name);

	envs->name = strdup(name);
}

int nemoenvs_set_callback(struct nemoenvs *envs, nemoenvs_callback_t callback, void *data)
{
	struct envscallback *cb;

	cb = (struct envscallback *)malloc(sizeof(struct envscallback));
	if (cb == NULL)
		return -1;

	cb->callback = callback;
	cb->data = data;

	nemolist_insert_tail(&envs->callback_list, &cb->link);

	return 0;
}

int nemoenvs_put_callback(struct nemoenvs *envs, nemoenvs_callback_t callback, void *data)
{
	struct envscallback *cb;

	nemolist_for_each(cb, &envs->callback_list, link) {
		if (cb->callback == callback) {
			nemolist_remove(&cb->link);

			free(cb);

			break;
		}
	}

	return 0;
}

int nemoenvs_dispatch(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one)
{
	struct envscallback *cb;

	nemolist_for_each(cb, &envs->callback_list, link) {
		cb->callback(envs, src, dst, cmd, path, one, cb->data);
	}

	return 0;
}

static int nemoenvs_handle_message(void *data)
{
	struct nemoenvs *envs = (struct nemoenvs *)data;
	struct nemomsg *msg = envs->msg;
	struct nemotoken *contents;
	struct itemone *one;
	const char *src;
	const char *dst;
	const char *cmd;
	const char *path;
	char buffer[1024];
	int size;
	int count;
	int i;

	size = nemomsg_recv_message(msg, buffer, sizeof(buffer) - 1);
	if (size <= 0)
		return -1;

	contents = nemotoken_create(buffer, size);
	nemotoken_divide(contents, ' ');
	nemotoken_divide(contents, '\t');
	nemotoken_divide(contents, '\n');
	nemotoken_update(contents);

	if (nemotoken_get_token_count(contents) < 4)
		return -1;

	src = nemotoken_get_token(contents, 0);
	dst = nemotoken_get_token(contents, 1);
	cmd = nemotoken_get_token(contents, 2);
	path = nemotoken_get_token(contents, 3);

	count = (nemotoken_get_token_count(contents) - 4) / 2;

	one = nemoitem_one_create();
	nemoitem_one_set_path(one, path);

	for (i = 0; i < count; i++) {
		nemoitem_one_set_attr(one,
				nemotoken_get_token(contents, 4 + i * 2 + 0),
				nemotoken_get_token(contents, 4 + i * 2 + 1));
	}

	nemoenvs_dispatch(envs, src, dst, cmd, path, one);

	nemoitem_one_destroy(one);

	nemotoken_destroy(contents);

	return 0;
}

static void nemoenvs_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct nemoenvs *envs = (struct nemoenvs *)data;

	nemomsg_clean_clients(envs->msg);
	nemomsg_check_clients(envs->msg);
	nemomsg_send_format(envs->msg, "/*", "/nemoshell /* get /check/live");

	nemotimer_set_timeout(envs->timer, NEMOENVS_LIVENESS_TIMEOUT);
}

int nemoenvs_listen(struct nemoenvs *envs, const char *ip, int port)
{
	envs->msg = nemomsg_create(ip, port);
	if (envs->msg == NULL)
		return -1;

	envs->monitor = nemomonitor_create(envs->shell->compz,
			nemomsg_get_socket(envs->msg),
			nemoenvs_handle_message,
			envs);

	envs->timer = nemotimer_create(envs->shell->compz);
	nemotimer_set_callback(envs->timer, nemoenvs_dispatch_timer);
	nemotimer_set_timeout(envs->timer, NEMOENVS_LIVENESS_TIMEOUT);
	nemotimer_set_userdata(envs->timer, envs);

	return 0;
}

int nemoenvs_send(struct nemoenvs *envs, const char *name, const char *fmt, ...)
{
	va_list vargs;
	int r;

	va_start(vargs, fmt);

	r = nemomsg_send_vargs(envs->msg, name, fmt, vargs);

	va_end(vargs);

	return r;
}

int nemoenvs_reply(struct nemoenvs *envs, const char *fmt, ...)
{
	va_list vargs;
	int r;

	va_start(vargs, fmt);

	r = nemomsg_sendto_vargs(envs->msg,
			nemomsg_get_source_ip(envs->msg),
			nemomsg_get_source_port(envs->msg),
			fmt,
			vargs);

	va_end(vargs);

	return r;
}

int nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath)
{
	struct itemone *one;
	FILE *fp;
	char buffer[1024];

	fp = fopen(configpath, "r");
	if (fp == NULL)
		return -1;

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (buffer[0] == '/' && buffer[1] == '/')
			continue;
		if (buffer[0] != '/')
			continue;

		string_replace(buffer, strlen(buffer), '\n', '\0');
		string_replace(buffer, strlen(buffer), '\t', ' ');

		one = nemoitem_one_create();
		nemoitem_one_load(one, buffer, ' ');

		nemoenvs_dispatch(envs, "/file", envs->name, "set", nemoitem_one_get_path(one), one);

		nemoitem_one_destroy(one);
	}

	fclose(fp);

	return 0;
}

int nemoenvs_save_configs(struct nemoenvs *envs, const char *configpath)
{
	struct itemone *one;
	FILE *fp;
	char buffer[1024];

	fp = fopen(configpath, "w");
	if (fp == NULL)
		return -1;

	nemoitem_for_each(one, envs->configs) {
		nemoitem_one_save(one, buffer, ' ');

		fputs(buffer, fp);
		fputc('\n', fp);
	}

	fclose(fp);

	return 0;
}
