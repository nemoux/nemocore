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

	nemolist_enqueue(&queue->list, &one->link);

	pthread_mutex_unlock(&queue->lock);
}

void nemoqueue_enqueue_one_tail(struct nemoqueue *queue, struct eventone *one)
{
	pthread_mutex_lock(&queue->lock);

	nemolist_enqueue_tail(&queue->list, &one->link);

	pthread_mutex_unlock(&queue->lock);
}

struct eventone *nemoqueue_dequeue_one(struct nemoqueue *queue)
{
	struct nemolist *elm;

	pthread_mutex_lock(&queue->lock);

	elm = nemolist_dequeue(&queue->list);

	pthread_mutex_unlock(&queue->lock);

	return elm != NULL ? container_of(elm, struct eventone, link) : NULL;
}

struct eventone *nemoqueue_one_create(int isize, int fsize)
{
	struct eventone *one;

	one = (struct eventone *)malloc(sizeof(struct eventone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct eventone));

	if (isize > 0) {
		one->iattrs = (int *)malloc(sizeof(int) * isize);
		one->niattrs = isize;
	}

	if (fsize > 0) {
		one->fattrs = (float *)malloc(sizeof(float) * fsize);
		one->nfattrs = fsize;
	}

	nemolist_init(&one->link);

	return one;
}

void nemoqueue_one_destroy(struct eventone *one)
{
	nemolist_remove(&one->link);

	if (one->iattrs != NULL)
		free(one->iattrs);
	if (one->fattrs != NULL)
		free(one->fattrs);

	free(one);
}
