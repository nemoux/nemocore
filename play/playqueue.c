#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoplay.h>
#include <playqueue.h>
#include <nemomisc.h>

struct playqueue *nemoplay_queue_create(void)
{
	struct playqueue *queue;

	queue = (struct playqueue *)malloc(sizeof(struct playqueue));
	if (queue == NULL)
		return NULL;
	memset(queue, 0, sizeof(struct playqueue));

	if (pthread_mutex_init(&queue->lock, NULL) != 0)
		goto err1;

	if (pthread_cond_init(&queue->signal, NULL) != 0)
		goto err2;

	nemolist_init(&queue->list);

	return queue;

err2:
	pthread_mutex_destroy(&queue->lock);

err1:
	free(queue);

	return NULL;
}

void nemoplay_queue_destroy(struct playqueue *queue)
{
	nemolist_remove(&queue->list);

	pthread_cond_destroy(&queue->signal);
	pthread_mutex_destroy(&queue->lock);

	free(queue);
}

struct playone *nemoplay_queue_create_one(void)
{
	struct playone *one;

	one = (struct playone *)malloc(sizeof(struct playone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct playone));

	nemolist_init(&one->link);

	return one;
}

void nemoplay_queue_destroy_one(struct playone *one)
{
	nemolist_remove(&one->link);

	free(one);
}

void nemoplay_queue_enqueue(struct playqueue *queue, struct playone *one)
{
	pthread_mutex_lock(&queue->lock);

	nemolist_enqueue(&queue->list, &one->link);

	pthread_mutex_unlock(&queue->lock);

	pthread_cond_signal(&queue->signal);
}

struct playone *nemoplay_queue_dequeue(struct playqueue *queue)
{
	struct nemolist *elm;

	pthread_mutex_lock(&queue->lock);

retry:
	elm = nemolist_dequeue(&queue->list);
	if (elm == NULL) {
		pthread_cond_wait(&queue->signal, &queue->lock);

		goto retry;
	}

	pthread_mutex_unlock(&queue->lock);

	return container_of(elm, struct playone, link);
}
