#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <compz.h>
#include <monitor.h>
#include <nemomisc.h>

static int nemomonitor_dispatch_event(int fd, uint32_t mask, void *data)
{
	struct nemomonitor *monitor = (struct nemomonitor *)data;

	if (monitor->callback(monitor->data) < 0)
		return 1;

	return 1;
}

struct nemomonitor *nemomonitor_create(struct nemocompz *compz, int fd, nemomonitor_callback_t callback, void *data)
{
	struct nemomonitor *monitor;

	monitor = (struct nemomonitor *)malloc(sizeof(struct nemomonitor));
	if (monitor == NULL)
		return NULL;

	monitor->callback = callback;
	monitor->data = data;

	monitor->source = wl_event_loop_add_fd(compz->loop, fd,
			WL_EVENT_READABLE,
			nemomonitor_dispatch_event,
			monitor);
	if (monitor->source == NULL)
		goto err1;

	return monitor;

err1:
	free(monitor);

	return NULL;
}

void nemomonitor_destroy(struct nemomonitor *monitor)
{
	wl_event_source_remove(monitor->source);

	free(monitor);
}
