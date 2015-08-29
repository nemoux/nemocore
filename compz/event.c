#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/eventfd.h>

#include <wayland-server.h>

#include <compz.h>
#include <event.h>
#include <nemomisc.h>

static int nemoevent_dispatch_event(int fd, uint32_t mask, void *data)
{
	struct nemoevent *event = (struct nemoevent *)data;
	struct nemocompz *compz = event->compz;
	struct eventone *one;
	uint64_t v;
	int r;

	r = read(event->eventfd, &v, sizeof(uint64_t));
	if (r != sizeof(uint64_t))
		return 1;

	while ((one = nemoevent_dequeue_one(event)) != NULL) {
		one->dispatch(compz, one->context, one->data);

		nemoevent_destroy_one(one);
	}

	return 1;
}

struct nemoevent *nemoevent_create(struct nemocompz *compz)
{
	struct nemoevent *event;

	event = (struct nemoevent *)malloc(sizeof(struct nemoevent));
	if (event == NULL)
		return NULL;
	memset(event, 0, sizeof(struct nemoevent));

	event->compz = compz;

	event->eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (event->eventfd < 0)
		goto err1;

	event->source = wl_event_loop_add_fd(compz->loop,
			event->eventfd,
			WL_EVENT_READABLE,
			nemoevent_dispatch_event,
			event);
	if (event->source == NULL)
		goto err2;

	if (pthread_mutex_init(&event->lock, NULL) != 0)
		goto err3;

	nemolist_init(&event->event_list);

	return event;

err3:
	wl_event_source_remove(event->source);

err2:
	close(event->eventfd);

err1:
	free(event);

	return NULL;
}

void nemoevent_destroy(struct nemoevent *event)
{
	nemolist_remove(&event->event_list);

	pthread_mutex_destroy(&event->lock);

	wl_event_source_remove(event->source);

	close(event->eventfd);

	free(event);
}

void nemoevent_trigger(struct nemoevent *event, uint64_t v)
{
	write(event->eventfd, &v, sizeof(uint64_t));
}

struct eventone *nemoevent_create_one(nemoevent_dispatch_t dispatch, void *context, void *data)
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

void nemoevent_destroy_one(struct eventone *one)
{
	nemolist_remove(&one->link);

	free(one);
}

void nemoevent_enqueue_one(struct nemoevent *event, struct eventone *one)
{
	pthread_mutex_lock(&event->lock);

	nemolist_insert(&event->event_list, &one->link);

	pthread_mutex_unlock(&event->lock);
}

struct eventone *nemoevent_dequeue_one(struct nemoevent *event)
{
	struct eventone *one = NULL;

	pthread_mutex_lock(&event->lock);

	if (nemolist_empty(&event->event_list) == 0) {
		one = nemolist_node0(&event->event_list, struct eventone, link);

		nemolist_remove(&one->link);
		nemolist_init(&one->link);
	}

	pthread_mutex_unlock(&event->lock);

	return one;
}
