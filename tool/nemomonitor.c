#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/epoll.h>

#include <nemomonitor.h>
#include <nemotool.h>
#include <nemomisc.h>

static void nemomonitor_dispatch_event(void *data, uint32_t events)
{
	struct nemomonitor *monitor = (struct nemomonitor *)data;

	monitor->callback(monitor->data);
}

struct nemomonitor *nemomonitor_create(struct nemotool *tool, int fd, nemomonitor_callback_t callback, void *data)
{
	struct nemomonitor *monitor;

	monitor = (struct nemomonitor *)malloc(sizeof(struct nemomonitor));
	if (monitor == NULL)
		return NULL;

	monitor->tool = tool;
	monitor->fd = fd;

	monitor->callback = callback;
	monitor->data = data;

	nemotool_watch_source(tool, fd, EPOLLIN | EPOLLERR | EPOLLHUP, nemomonitor_dispatch_event, monitor);

	return monitor;

err1:
	free(monitor);

	return NULL;
}

void nemomonitor_destroy(struct nemomonitor *monitor)
{
	nemotool_unwatch_source(monitor->tool, monitor->fd);

	free(monitor);
}
