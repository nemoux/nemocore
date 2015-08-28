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
	uint64_t v;
	int r;

	r = read(event->eventfd, &v, sizeof(uint64_t));
	if (r != sizeof(uint64_t))
		return 1;

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

	return event;

err2:
	close(event->eventfd);

err1:
	free(event);

	return NULL;
}

void nemoevent_destroy(struct nemoevent *event)
{
	if (event->source != NULL)
		wl_event_source_remove(event->source);

	close(event->eventfd);

	free(event);
}

void nemoevent_write(struct nemoevent *event, uint64_t v)
{
	write(event->eventfd, &v, sizeof(uint64_t));
}

uint64_t nemoevent_read(struct nemoevent *event)
{
	uint64_t v;
	int r;

	r = read(event->eventfd, &v, sizeof(uint64_t));
	if (r != sizeof(uint64_t))
		return 0;

	return v;
}
