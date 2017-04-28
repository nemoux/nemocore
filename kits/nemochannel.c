#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/eventfd.h>

#include <wayland-client.h>

#include <nemochannel.h>
#include <nemomisc.h>

struct nemochannel *nemochannel_create(void)
{
	struct nemochannel *chan;

	chan = (struct nemochannel *)malloc(sizeof(struct nemochannel));
	if (chan == NULL)
		return NULL;
	memset(chan, 0, sizeof(struct nemochannel));

	chan->eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (chan->eventfd < 0)
		goto err1;

	return chan;

err1:
	free(chan);

	return NULL;
}

void nemochannel_destroy(struct nemochannel *chan)
{
	close(chan->eventfd);

	free(chan);
}

void nemochannel_set_blocking_mode(struct nemochannel *chan, int is_blocking)
{
	if (is_blocking != 0)
		os_fd_put_nonblocking_mode(chan->eventfd);
	else
		os_fd_set_nonblocking_mode(chan->eventfd);
}

uint64_t nemochannel_read(struct nemochannel *chan)
{
	uint64_t event;
	int r;

	r = read(chan->eventfd, &event, sizeof(event));
	if (r != sizeof(event))
		return 0;

	return event;
}

void nemochannel_write(struct nemochannel *chan, uint64_t event)
{
	write(chan->eventfd, &event, sizeof(event));
}
