#ifndef	__NEMO_TIMER_H__
#define	__NEMO_TIMER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemocompz;
struct nemotimer;

typedef void (*nemotimer_dispatch_t)(struct nemotimer *timer, void *data);

struct nemotimer {
	struct nemocompz *compz;

	struct wl_event_source *source;

	nemotimer_dispatch_t callback;

	void *userdata;
};

extern struct nemotimer *nemotimer_create(struct nemocompz *compz);
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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
