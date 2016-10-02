#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemomisc.h>

static void nemotimer_dispatch_event(void *data, const char *events)
{
	struct nemotimer *timer = (struct nemotimer *)data;
	uint64_t exp;

	if (read(timer->timer_fd, &exp, sizeof(exp)) != sizeof(exp))
		return;

	if (timer->callback != NULL)
		timer->callback(timer, timer->userdata);
}

struct nemotimer *nemotimer_create(struct nemotool *tool)
{
	struct nemotimer *timer;

	timer = (struct nemotimer *)malloc(sizeof(struct nemotimer));
	if (timer == NULL)
		return NULL;
	memset(timer, 0, sizeof(struct nemotimer));

	timer->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	if (timer->timer_fd < 0)
		goto err1;

	timer->tool = tool;

	nemotool_watch_source(tool, timer->timer_fd, "r", nemotimer_dispatch_event, timer);

	return timer;

err1:
	free(timer);

	return NULL;
}

void nemotimer_destroy(struct nemotimer *timer)
{
	nemotool_unwatch_source(timer->tool, timer->timer_fd);

	close(timer->timer_fd);

	free(timer);
}

void nemotimer_set_callback(struct nemotimer *timer, nemotimer_dispatch_t callback)
{
	timer->callback = callback;
}

void nemotimer_set_timeout(struct nemotimer *timer, int32_t msecs)
{
	struct itimerspec its;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = msecs / 1000;
	its.it_value.tv_nsec = (msecs % 1000) * 1000 * 1000;

	timerfd_settime(timer->timer_fd, 0, &its, NULL);
}
