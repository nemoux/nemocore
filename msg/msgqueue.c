#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <msgqueue.h>

struct msgqueue *nemomsg_queue_create(void)
{
	struct msgqueue *queue;

	queue = (struct msgqueue *)malloc(sizeof(struct msgqueue));
	if (queue == NULL)
		return NULL;
	memset(queue, 0, sizeof(struct msgqueue));

	nemolist_init(&queue->callback_list);
	nemolist_init(&queue->source_list);
	nemolist_init(&queue->destination_list);
	nemolist_init(&queue->command_list);

	nemolist_init(&queue->delete_list);

	return queue;
}

void nemomsg_queue_destroy(struct msgqueue *queue)
{
	struct msgcallback *cb, *next;

	nemolist_for_each_safe(cb, next, &queue->callback_list, link) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	nemolist_for_each_safe(cb, next, &queue->source_list, link) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	nemolist_for_each_safe(cb, next, &queue->destination_list, link) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	nemolist_for_each_safe(cb, next, &queue->command_list, link) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	nemolist_for_each_safe(cb, next, &queue->delete_list, link) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	free(queue);
}

int nemomsg_queue_set_callback(struct msgqueue *queue, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	cb = (struct msgcallback *)malloc(sizeof(struct msgcallback));
	if (cb == NULL)
		return -1;

	cb->name = strdup("");
	cb->callback = callback;

	nemolist_init(&cb->dlink);

	nemolist_insert_tail(&queue->callback_list, &cb->link);

	return 0;
}

int nemomsg_queue_set_source_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	cb = (struct msgcallback *)malloc(sizeof(struct msgcallback));
	if (cb == NULL)
		return -1;

	cb->name = strdup(name);
	cb->callback = callback;

	nemolist_init(&cb->dlink);

	nemolist_insert_tail(&queue->source_list, &cb->link);

	return 0;
}

int nemomsg_queue_set_destination_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	cb = (struct msgcallback *)malloc(sizeof(struct msgcallback));
	if (cb == NULL)
		return -1;

	cb->name = strdup(name);
	cb->callback = callback;

	nemolist_init(&cb->dlink);

	nemolist_insert_tail(&queue->destination_list, &cb->link);

	return 0;
}

int nemomsg_queue_set_command_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	cb = (struct msgcallback *)malloc(sizeof(struct msgcallback));
	if (cb == NULL)
		return -1;

	cb->name = strdup(name);
	cb->callback = callback;

	nemolist_init(&cb->dlink);

	nemolist_insert_tail(&queue->command_list, &cb->link);

	return 0;
}

int nemomsg_queue_put_callback(struct msgqueue *queue, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	nemolist_for_each(cb, &queue->callback_list, link) {
		if (cb->callback == callback) {
			nemolist_insert(&queue->delete_list, &cb->dlink);
		}
	}

	return 0;
}

int nemomsg_queue_put_source_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	nemolist_for_each(cb, &queue->source_list, link) {
		if (strcmp(cb->name, name) == 0 && cb->callback == callback) {
			nemolist_insert(&queue->delete_list, &cb->dlink);
		}
	}

	return 0;
}

int nemomsg_queue_put_destination_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	nemolist_for_each(cb, &queue->destination_list, link) {
		if (strcmp(cb->name, name) == 0 && cb->callback == callback) {
			nemolist_insert(&queue->delete_list, &cb->dlink);
		}
	}

	return 0;
}

int nemomsg_queue_put_command_callback(struct msgqueue *queue, const char *name, nemomsg_callback_t callback)
{
	struct msgcallback *cb;

	nemolist_for_each(cb, &queue->command_list, link) {
		if (strcmp(cb->name, name) == 0 && cb->callback == callback) {
			nemolist_insert(&queue->delete_list, &cb->dlink);
		}
	}

	return 0;
}

int nemomsg_queue_dispatch(struct msgqueue *queue, struct nemotoken *msg)
{
	struct msgcallback *cb;
	const char *src;
	const char *dst;
	const char *cmd;

	src = nemotoken_get_token(msg, 0);
	dst = nemotoken_get_token(msg, 1);
	cmd = nemotoken_get_token(msg, 2);

	nemolist_for_each(cb, &queue->callback_list, link) {
		cb->callback(queue->data, cmd, msg);
	}

	nemolist_for_each(cb, &queue->source_list, link) {
		if (strcmp(cb->name, src) == 0)
			cb->callback(queue->data, src, msg);
	}

	nemolist_for_each(cb, &queue->destination_list, link) {
		if (strcmp(cb->name, dst) == 0)
			cb->callback(queue->data, dst, msg);
	}

	nemolist_for_each(cb, &queue->command_list, link) {
		if (strcmp(cb->name, cmd) == 0)
			cb->callback(queue->data, cmd, msg);
	}

	return 0;
}

int nemomsg_queue_clean(struct msgqueue *queue)
{
	struct msgcallback *cb, *next;

	nemolist_for_each_safe(cb, next, &queue->delete_list, dlink) {
		nemolist_remove(&cb->link);
		nemolist_remove(&cb->dlink);

		free(cb->name);
		free(cb);
	}

	return 0;
}
