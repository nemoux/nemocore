#ifndef	__NEMOTOOL_TIMER_H__
#define	__NEMOTOOL_TIMER_H__

struct nemotool;
struct nemotimer;
struct nemotask;

typedef void (*nemotimer_dispatch_t)(struct nemotimer *timer, void *data);

struct nemotimer {
	struct nemotool *tool;

	int timer_fd;
	struct nemotask timer_task;

	nemotimer_dispatch_t callback;

	void *userdata;
};

extern struct nemotimer *nemotimer_create(struct nemotool *tool);
extern void nemotimer_destroy(struct nemotimer *timer);

extern void nemotimer_set_callback(struct nemotimer *timer, nemotimer_dispatch_t callback);
extern void nemotimer_set_timeout(struct nemotimer *timer, int32_t msecs);

static inline void nemotimer_set_userdata(struct nemotimer *timer, void *data)
{
	timer->userdata = data;
}

static inline void *nemotimer_get_userdata(struct nemotimer *timer)
{
	return timer->userdata;
}

#endif
