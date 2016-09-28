#define _GNU_SOURCE
#define __USE_GNU
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

static int nemoenvs_dispatch_client_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data)
{
	struct nemotool *tool = (struct nemotool *)data;

	if (strcmp(cmd, "req") == 0) {
		if (strcmp(path, "/check/live") == 0) {
			nemoenvs_reply(envs, "%s %s rep /check/live", envs->clientname, src);
		}
	}

	return 0;
}

struct nemoenvs *nemoenvs_create(struct nemotool *tool)
{
	struct nemoenvs *envs;

	envs = (struct nemoenvs *)malloc(sizeof(struct nemoenvs));
	if (envs == NULL)
		return NULL;
	memset(envs, 0, sizeof(struct nemoenvs));

	envs->tool = tool;

	nemolist_init(&envs->callback_list);

	nemoenvs_set_callback(envs, nemoenvs_dispatch_client_message, tool);

	return envs;
}

void nemoenvs_destroy(struct nemoenvs *envs)
{
	struct envscallback *cb, *next;

	nemolist_for_each_safe(cb, next, &envs->callback_list, link) {
		nemolist_remove(&cb->link);

		free(cb);
	}

	if (envs->monitor != NULL)
		nemomonitor_destroy(envs->monitor);
	if (envs->msg != NULL)
		nemomsg_destroy(envs->msg);

	if (envs->clientname != NULL)
		free(envs->clientname);
	if (envs->servername != NULL)
		free(envs->servername);
	if (envs->id != NULL)
		free(envs->id);

	free(envs);
}

void nemoenvs_set_id(struct nemoenvs *envs, const char *id)
{
	if (envs->id != NULL)
		free(envs->id);

	if (id != NULL)
		envs->id = strdup(id);
	else
		envs->id = NULL;
}

void nemoenvs_set_name(struct nemoenvs *envs, const char *fmt, ...)
{
	va_list vargs;

	if (envs->clientname != NULL)
		free(envs->clientname);

	va_start(vargs, fmt);
	vasprintf(&envs->clientname, fmt, vargs);
	va_end(vargs);
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
	nemotoken_fence(contents, '\"');
	nemotoken_divide(contents, ' ');
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

int nemoenvs_connect(struct nemoenvs *envs, const char *servername, const char *ip, int port)
{
	envs->msg = nemomsg_create(NULL, 0);
	if (envs->msg == NULL)
		return -1;

	envs->servername = strdup(servername);

	envs->monitor = nemomonitor_create(envs->tool,
			nemomsg_get_socket(envs->msg),
			nemoenvs_handle_message,
			envs);

	nemomsg_set_client(envs->msg, servername, ip, port);
	nemomsg_send_format(envs->msg, servername, "%s %s rep /check/live", envs->clientname, servername);

	return 0;
}

int nemoenvs_send(struct nemoenvs *envs, const char *fmt, ...)
{
	va_list vargs;
	int r;

	va_start(vargs, fmt);

	r = nemomsg_send_vargs(envs->msg, envs->servername, fmt, vargs);

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
