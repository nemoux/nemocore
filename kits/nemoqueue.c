#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include <nemoqueue.h>
#include <nemolist.h>
#include <nemomisc.h>

struct eventone {
	int *iattrs;
	int niattrs;

	float *fattrs;
	int nfattrs;

	char **sattrs;
	int nsattrs;

	void **pattrs;
	int npattrs;

	struct nemolist link;
};

struct nemoqueue {
	struct nemolist list;
	pthread_mutex_t lock;
};

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

struct eventone *nemoqueue_one_create(void)
{
	struct eventone *one;

	one = (struct eventone *)malloc(sizeof(struct eventone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct eventone));

	nemolist_init(&one->link);

	return one;
}

void nemoqueue_one_destroy(struct eventone *one)
{
	int i;

	nemolist_remove(&one->link);

	if (one->iattrs != NULL)
		free(one->iattrs);
	if (one->fattrs != NULL)
		free(one->fattrs);
	if (one->sattrs != NULL) {
		for (i = 0; i < one->nsattrs; i++)
			free(one->sattrs[i]);

		free(one->sattrs);
	}
	if (one->pattrs != NULL)
		free(one->pattrs);

	free(one);
}

void nemoqueue_one_set_max_integer(struct eventone *one, int count)
{
	one->iattrs = (int *)malloc(sizeof(int) * count);
	one->niattrs = count;
}

void nemoqueue_one_set_max_float(struct eventone *one, int count)
{
	one->fattrs = (float *)malloc(sizeof(float) * count);
	one->nfattrs = count;
}

void nemoqueue_one_set_max_string(struct eventone *one, int count)
{
	one->sattrs = (char **)malloc(sizeof(char *) * count);
	one->nsattrs = count;
}

void nemoqueue_one_set_max_pointer(struct eventone *one, int count)
{
	one->pattrs = (void **)malloc(sizeof(void *) * count);
	one->npattrs = count;
}

int nemoqueue_one_get_max_integer(struct eventone *one)
{
	return one->niattrs;
}

int nemoqueue_one_get_max_float(struct eventone *one)
{
	return one->nfattrs;
}

int nemoqueue_one_get_max_string(struct eventone *one)
{
	return one->nsattrs;
}

int nemoqueue_one_get_max_pointer(struct eventone *one)
{
	return one->npattrs;
}

void nemoqueue_one_set_integer(struct eventone *one, int index, int attr)
{
	one->iattrs[index] = attr;
}

int nemoqueue_one_get_integer(struct eventone *one, int index)
{
	return one->iattrs[index];
}

void nemoqueue_one_set_float(struct eventone *one, int index, float attr)
{
	one->fattrs[index] = attr;
}

float nemoqueue_one_get_float(struct eventone *one, int index)
{
	return one->fattrs[index];
}

void nemoqueue_one_set_string(struct eventone *one, int index, const char *attr)
{
	one->sattrs[index] = strdup(attr);
}

const char *nemoqueue_one_get_string(struct eventone *one, int index)
{
	return one->sattrs[index];
}

void nemoqueue_one_set_pointer(struct eventone *one, int index, void *attr)
{
	one->pattrs[index] = attr;
}

void *nemoqueue_one_get_pointer(struct eventone *one, int index)
{
	return one->pattrs[index];
}
