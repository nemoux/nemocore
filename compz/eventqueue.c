#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/eventfd.h>

#include <wayland-server.h>

#include <compz.h>
#include <eventqueue.h>
#include <nemomisc.h>

static int nemoeventqueue_dispatch_event(int fd, uint32_t mask, void *data)
{
	struct nemoeventqueue *queue = (struct nemoeventqueue *)data;
	struct nemocompz *compz = queue->compz;
	struct eventone *one;
	uint64_t v;
	int r;

	r = read(queue->eventfd, &v, sizeof(uint64_t));
	if (r != sizeof(uint64_t))
		return 1;

	while ((one = nemoeventqueue_dequeue_one(queue)) != NULL) {
		one->dispatch(compz, one->context, one->data);

		nemoeventqueue_destroy_one(one);
	}

	return 1;
}

struct nemoeventqueue *nemoeventqueue_create(struct nemocompz *compz)
{
	struct nemoeventqueue *queue;

	queue = (struct nemoeventqueue *)malloc(sizeof(struct nemoeventqueue));
	if (queue == NULL)
		return NULL;
	memset(queue, 0, sizeof(struct nemoeventqueue));

	queue->compz = compz;

	queue->eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (queue->eventfd < 0)
		goto err1;

	queue->source = wl_event_loop_add_fd(compz->loop,
			queue->eventfd,
			WL_EVENT_READABLE,
			nemoeventqueue_dispatch_event,
			queue);
	if (queue->source == NULL)
		goto err2;

	if (pthread_mutex_init(&queue->lock, NULL) != 0)
		goto err3;

	nemolist_init(&queue->event_list);

	return queue;

err3:
	wl_event_source_remove(queue->source);

err2:
	close(queue->eventfd);

err1:
	free(queue);

	return NULL;
}

void nemoeventqueue_destroy(struct nemoeventqueue *queue)
{
	nemolist_remove(&queue->event_list);

	pthread_mutex_destroy(&queue->lock);

	wl_event_source_remove(queue->source);

	close(queue->eventfd);

	free(queue);
}

void nemoeventqueue_trigger(struct nemoeventqueue *queue, uint64_t v)
{
	write(queue->eventfd, &v, sizeof(uint64_t));
}

struct eventone *nemoeventqueue_create_one(nemoeventqueue_dispatch_t dispatch, void *context, void *data)
{
	struct eventone *one;

	one = (struct eventone *)malloc(sizeof(struct eventone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct eventone));

	one->dispatch = dispatch;
	one->context = context;
	one->data = data;

	nemolist_init(&one->link);

	return one;
}

void nemoeventqueue_destroy_one(struct eventone *one)
{
	nemolist_remove(&one->link);

	free(one);
}

void nemoeventqueue_enqueue_one(struct nemoeventqueue *queue, struct eventone *one)
{
	pthread_mutex_lock(&queue->lock);

	nemolist_insert(&queue->event_list, &one->link);

	pthread_mutex_unlock(&queue->lock);
}

struct eventone *nemoeventqueue_dequeue_one(struct nemoeventqueue *queue)
{
	struct eventone *one = NULL;

	pthread_mutex_lock(&queue->lock);

	if (nemolist_empty(&queue->event_list) == 0) {
		one = nemolist_node0(&queue->event_list, struct eventone, link);

		nemolist_remove(&one->link);
		nemolist_init(&one->link);
	}

	pthread_mutex_unlock(&queue->lock);

	return one;
}
