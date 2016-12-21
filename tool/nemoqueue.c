#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoqueue.h>
#include <nemomisc.h>

struct nemoqueue *nemoqueue_create(void)
{
	struct nemoqueue *queue;

	queue = (struct nemoqueue *)malloc(sizeof(struct nemoqueue));
	if (queue == NULL)
		return NULL;
	memset(queue, 0, sizeof(struct nemoqueue));

	nemolist_init(&queue->list);

	if (pthread_mutex_init(&queue->lock, NULL) != 0)
		goto err1;

	return queue;

err1:
	free(queue);

	return NULL;
}

void nemoqueue_destroy(struct nemoqueue *queue)
{
	struct eventone *one, *next;

	nemolist_for_each_safe(one, next, &queue->list, link) {
		nemoqueue_one_destroy(one);
	}

	pthread_mutex_destroy(&queue->lock);

	free(queue);
}

void nemoqueue_enqueue_one(struct nemoqueue *queue, struct eventone *one)
{
	pthread_mutex_lock(&queue->lock);

	nemolist_insert(&queue->list, &one->link);

	pthread_mutex_unlock(&queue->lock);
}

struct eventone *nemoqueue_dequeue_one(struct nemoqueue *queue)
{
	struct eventone *one = NULL;

	pthread_mutex_lock(&queue->lock);

	if (nemolist_empty(&queue->list) == 0) {
		one = nemolist_node0(&queue->list, struct eventone, link);

		nemolist_remove(&one->link);
		nemolist_init(&one->link);
	}

	pthread_mutex_unlock(&queue->lock);

	return one;
}

struct eventone *nemoqueue_one_create(uint32_t type, uint32_t tag, int size)
{
	struct eventone *one;

	one = (struct eventone *)malloc(sizeof(struct eventone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct eventone));

	one->type = type;
	one->tag = tag;

	one->attrs = (float *)malloc(sizeof(float) * size);
	one->nattrs = size;

	nemolist_init(&one->link);

	return one;
}

void nemoqueue_one_destroy(struct eventone *one)
{
	nemolist_remove(&one->link);

	free(one->attrs);
	free(one);
}
