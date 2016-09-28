#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <regex.h>

#include <nemomsg.h>
#include <udphelper.h>
#include <nemomisc.h>

struct nemomsg *nemomsg_create(const char *ip, int port)
{
	struct nemomsg *msg;

	msg = (struct nemomsg *)malloc(sizeof(struct nemomsg));
	if (msg == NULL)
		return NULL;
	memset(msg, 0, sizeof(struct nemomsg));

	msg->soc = udp_create_socket(ip, port);
	if (msg->soc < 0)
		goto err1;

	nemolist_init(&msg->client_list);

	nemolist_init(&msg->callback_list);
	nemolist_init(&msg->source_list);
	nemolist_init(&msg->destination_list);

	nemolist_init(&msg->delete_list);

	return msg;

err1:
	free(msg);

	return NULL;
}

void nemomsg_destroy(struct nemomsg *msg)
{
	struct msgcallback *cb, *next;
	struct msgclient *client, *cnext;

	nemolist_for_each_safe(cb, next, &msg->callback_list, link) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	nemolist_for_each_safe(cb, next, &msg->source_list, link) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	nemolist_for_each_safe(cb, next, &msg->destination_list, link) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	nemolist_for_each_safe(cb, next, &msg->delete_list, link) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	nemolist_for_each_safe(client, cnext, &msg->client_list, link) {
		nemolist_remove(&client->link);

		free(client->name);
		free(client->ip);
		free(client);
	}

	close(msg->soc);

	free(msg);
}

int nemomsg_set_callback(struct nemomsg *msg, nemomsg_callback_t callback, void *data)
{
	struct msgcallback *cb;

	cb = (struct msgcallback *)malloc(sizeof(struct msgcallback));
	if (cb == NULL)
		return -1;

	cb->name = strdup("");
	cb->callback = callback;
	cb->data = data;

	nemolist_init(&cb->dlink);

	nemolist_insert_tail(&msg->callback_list, &cb->link);

	return 0;
}

int nemomsg_set_source_callback(struct nemomsg *msg, const char *name, nemomsg_callback_t callback, void *data)
{
	struct msgcallback *cb;

	cb = (struct msgcallback *)malloc(sizeof(struct msgcallback));
	if (cb == NULL)
		return -1;

	cb->name = strdup(name);
	cb->callback = callback;
	cb->data = data;

	nemolist_init(&cb->dlink);

	nemolist_insert_tail(&msg->source_list, &cb->link);

	return 0;
}

int nemomsg_set_destination_callback(struct nemomsg *msg, const char *name, nemomsg_callback_t callback, void *data)
{
	struct msgcallback *cb;

	cb = (struct msgcallback *)malloc(sizeof(struct msgcallback));
	if (cb == NULL)
		return -1;

	cb->name = strdup(name);
	cb->callback = callback;
	cb->data = data;

	nemolist_init(&cb->dlink);

	nemolist_insert_tail(&msg->destination_list, &cb->link);

	return 0;
}

int nemomsg_put_callback(struct nemomsg *msg, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	nemolist_for_each(cb, &msg->callback_list, link) {
		if (cb->callback == callback) {
			nemolist_insert(&msg->delete_list, &cb->dlink);
		}
	}

	return 0;
}

int nemomsg_put_source_callback(struct nemomsg *msg, const char *name, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	nemolist_for_each(cb, &msg->source_list, link) {
		if (strcmp(cb->name, name) == 0 && cb->callback == callback) {
			nemolist_insert(&msg->delete_list, &cb->dlink);
		}
	}

	return 0;
}

int nemomsg_put_destination_callback(struct nemomsg *msg, const char *name, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	nemolist_for_each(cb, &msg->destination_list, link) {
		if (strcmp(cb->name, name) == 0 && cb->callback == callback) {
			nemolist_insert(&msg->delete_list, &cb->dlink);
		}
	}

	return 0;
}

int nemomsg_dispatch(struct nemomsg *msg, const char *ip, int port, struct nemotoken *contents)
{
	struct msgcallback *cb;
	const char *src;
	const char *dst;

	if (nemotoken_get_token_count(contents) < 3)
		return -1;

	src = nemotoken_get_token(contents, 0);
	dst = nemotoken_get_token(contents, 1);

	nemolist_for_each(cb, &msg->callback_list, link) {
		cb->callback(cb->data, src, dst, contents);
	}

	nemolist_for_each(cb, &msg->source_list, link) {
		if (strcmp(cb->name, src) == 0)
			cb->callback(cb->data, src, dst, contents);
	}

	nemolist_for_each(cb, &msg->destination_list, link) {
		if (strcmp(cb->name, dst) == 0)
			cb->callback(cb->data, src, dst, contents);
	}

	return 0;
}

int nemomsg_clean(struct nemomsg *msg)
{
	struct msgcallback *cb, *next;

	nemolist_for_each_safe(cb, next, &msg->delete_list, dlink) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	return 0;
}

int nemomsg_set_client(struct nemomsg *msg, const char *name, const char *ip, int port)
{
	struct msgclient *client;

	nemolist_for_each(client, &msg->client_list, link) {
		if (strcmp(client->name, name) == 0 &&
				strcmp(client->ip, ip) == 0 &&
				client->port == port) {
			return 1;
		}
	}

	client = (struct msgclient *)malloc(sizeof(struct msgclient));
	if (client == NULL)
		return -1;

	client->name = strdup(name);
	client->ip = strdup(ip);
	client->port = port;

	nemolist_insert_tail(&msg->client_list, &client->link);

	return 0;
}

int nemomsg_put_client(struct nemomsg *msg, const char *name, const char *ip, int port)
{
	struct msgclient *client;

	nemolist_for_each(client, &msg->client_list, link) {
		if (strcmp(client->name, name) == 0 &&
				strcmp(client->ip, ip) == 0 &&
				client->port == port) {
			nemolist_remove(&client->link);

			free(client->name);
			free(client->ip);
			free(client);

			break;
		}
	}

	return 0;
}

int nemomsg_recv_message(struct nemomsg *msg, char *contents, int size)
{
	msg->source.buffer = contents;
	msg->source.size = udp_recv_from(msg->soc, msg->source.ip, &msg->source.port, contents, size);

	return msg->source.size;
}

int nemomsg_send_message(struct nemomsg *msg, const char *name, const char *contents, int size)
{
	struct msgclient *client;
	regex_t regex;

	if (regcomp(&regex, name, REG_EXTENDED))
		return -1;

	nemolist_for_each(client, &msg->client_list, link) {
		if (regexec(&regex, client->name, 0, NULL, 0) == 0) {
			udp_send_to(msg->soc, client->ip, client->port, contents, size);
		}
	}

	regfree(&regex);

	return 0;
}

int nemomsg_send_format(struct nemomsg *msg, const char *name, const char *fmt, ...)
{
	struct msgclient *client;
	regex_t regex;
	va_list vargs;
	char *contents;
	int size;

	if (regcomp(&regex, name, REG_EXTENDED))
		return -1;

	va_start(vargs, fmt);
	vasprintf(&contents, fmt, vargs);
	va_end(vargs);

	size = strlen(contents) + 1;

	nemolist_for_each(client, &msg->client_list, link) {
		if (regexec(&regex, client->name, 0, NULL, 0) == 0) {
			udp_send_to(msg->soc, client->ip, client->port, contents, size);
		}
	}

	free(contents);

	regfree(&regex);

	return 0;
}

int nemomsg_send_vargs(struct nemomsg *msg, const char *name, const char *fmt, va_list vargs)
{
	struct msgclient *client;
	regex_t regex;
	char *contents;
	int size;

	if (regcomp(&regex, name, REG_EXTENDED))
		return -1;

	vasprintf(&contents, fmt, vargs);

	size = strlen(contents) + 1;

	nemolist_for_each(client, &msg->client_list, link) {
		if (regexec(&regex, client->name, 0, NULL, 0) == 0) {
			udp_send_to(msg->soc, client->ip, client->port, contents, size);
		}
	}

	free(contents);

	regfree(&regex);

	return 0;
}

int nemomsg_sendto_message(struct nemomsg *msg, const char *ip, int port, const char *contents, int size)
{
	udp_send_to(msg->soc, ip, port, contents, size);

	return 0;
}

int nemomsg_sendto_format(struct nemomsg *msg, const char *ip, int port, const char *fmt, ...)
{
	va_list vargs;
	char *contents;

	va_start(vargs, fmt);
	vasprintf(&contents, fmt, vargs);
	va_end(vargs);

	udp_send_to(msg->soc, ip, port, contents, strlen(contents) + 1);

	free(contents);

	return 0;
}

int nemomsg_sendto_vargs(struct nemomsg *msg, const char *ip, int port, const char *fmt, va_list vargs)
{
	char *contents;

	vasprintf(&contents, fmt, vargs);

	udp_send_to(msg->soc, ip, port, contents, strlen(contents) + 1);

	free(contents);

	return 0;
}
