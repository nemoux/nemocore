#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <timer.h>
#include <compz.h>
#include <nemomisc.h>

static int nemotimer_dispatch_event(void *data)
{
	struct nemotimer *timer = (struct nemotimer *)data;

	timer->callback(timer, timer->userdata);

	return 1;
}

struct nemotimer *nemotimer_create(struct nemocompz *compz)
{
	struct nemotimer *timer;

	timer = (struct nemotimer *)malloc(sizeof(struct nemotimer));
	if (timer == NULL)
		return NULL;
	memset(timer, 0, sizeof(struct nemotimer));

	timer->compz = compz;

	timer->source = wl_event_loop_add_timer(compz->loop, nemotimer_dispatch_event, timer);
	if (timer->source == NULL)
		goto err1;

	return timer;

err1:
	free(timer);

	return NULL;
}

void nemotimer_destroy(struct nemotimer *timer)
{
	wl_event_source_remove(timer->source);

	free(timer);
}

void nemotimer_set_callback(struct nemotimer *timer, nemotimer_dispatch_t callback)
{
	timer->callback = callback;
}

void nemotimer_set_timeout(struct nemotimer *timer, int32_t msecs)
{
	wl_event_source_timer_update(timer->source, msecs);
}
