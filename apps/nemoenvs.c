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

struct nemoenvs *nemoenvs_create(struct nemotool *tool)
{
	struct nemoenvs *envs;

	envs = (struct nemoenvs *)malloc(sizeof(struct nemoenvs));
	if (envs == NULL)
		return NULL;
	memset(envs, 0, sizeof(struct nemoenvs));

	envs->tool = tool;

	envs->configs = nemoitem_create();
	if (envs->configs == NULL)
		goto err1;

	nemolist_init(&envs->callback_list);

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

	nemoitem_destroy(envs->configs);

	free(envs);
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

static int nemoenvs_dispatch_message(void *data)
{
	struct nemoenvs *envs = (struct nemoenvs *)data;
	struct nemomsg *msg = envs->msg;
	struct nemotoken *content;
	struct itemone *one;
	int soc = nemomsg_get_socket(msg);
	const char *src;
	const char *dst;
	const char *cmd;
	const char *path;
	char buffer[1024];
	char ip[64];
	int port;
	int size;
	int count;
	int i;

	size = udp_recv_from(soc, ip, &port, buffer, sizeof(buffer) - 1);
	if (size <= 0)
		return -1;

	content = nemotoken_create(buffer, size);
	nemotoken_divide(content, ':');
	nemotoken_update(content);

	if (nemotoken_get_token_count(content) < 4)
		return -1;

	src = nemotoken_get_token(content, 0);
	dst = nemotoken_get_token(content, 1);
	cmd = nemotoken_get_token(content, 2);
	path = nemotoken_get_token(content, 3);

	count = (nemotoken_get_token_count(content) - 4) / 2;

	if (strcmp(cmd, "in") == 0)
		nemomsg_set_client(msg, src, ip, port);
	else if (strcmp(cmd, "out") == 0)
		nemomsg_put_client(msg, src, ip, port);

	one = nemoitem_one_create();
	nemoitem_one_set_path(one, path);

	for (i = 0; i < count; i++) {
		nemoitem_one_set_attr(one,
				nemotoken_get_token(content, 4 + i * 2 + 0),
				nemotoken_get_token(content, 4 + i * 2 + 1));
	}

	nemoenvs_dispatch(envs, src, dst, cmd, path, one);

	nemoitem_one_destroy(one);

	nemotoken_destroy(content);

	return 0;
}

int nemoenvs_connect(struct nemoenvs *envs, const char *ip, int port)
{
	envs->msg = nemomsg_create(NULL, 0);
	if (envs->msg == NULL)
		return -1;

	envs->monitor = nemomonitor_create(envs->tool,
			nemomsg_get_socket(envs->msg),
			nemoenvs_dispatch_message,
			envs);

	nemomsg_set_client(envs->msg, "/nemoshell", ip, port);

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
