#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/eventfd.h>

#include <wayland-client.h>

#include <nemotool.h>
#include <nemochannel.h>
#include <nemomisc.h>

static void nemochannel_dispatch_event(void *data, const char *events)
{
	struct nemochannel *chan = (struct nemochannel *)data;
	uint64_t event;
	int r;

	r = read(chan->eventfd, &event, sizeof(event));
	if (r != sizeof(event))
		return;

	if (chan->dispatch_event != NULL)
		chan->dispatch_event(chan, event, chan->userdata);
}

struct nemochannel *nemochannel_create(struct nemotool *tool)
{
	struct nemochannel *chan;

	chan = (struct nemochannel *)malloc(sizeof(struct nemochannel));
	if (chan == NULL)
		return NULL;
	memset(chan, 0, sizeof(struct nemochannel));

	chan->tool = tool;

	chan->eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (chan->eventfd < 0)
		goto err1;

	nemotool_watch_source(tool, chan->eventfd, "r", nemochannel_dispatch_event, chan);

	return chan;

err1:
	free(chan);

	return NULL;
}

void nemochannel_destroy(struct nemochannel *chan)
{
	nemotool_unwatch_source(chan->tool, chan->eventfd);
	close(chan->eventfd);

	free(chan);
}

void nemochannel_dispatch(struct nemochannel *chan, uint64_t event)
{
	write(chan->eventfd, &event, sizeof(event));
}
