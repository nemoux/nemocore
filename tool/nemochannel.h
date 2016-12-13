#ifndef __NEMOTOOL_CANVAS_H__
#define	__NEMOTOOL_CANVAS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotool.h>

struct nemochannel;

typedef int (*nemochannel_dispatch_event_t)(struct nemochannel *chan, uint64_t event, void *data);

struct nemochannel {
	struct nemotool *tool;

	int eventfd;

	nemochannel_dispatch_event_t dispatch_event;

	void *userdata;
};

extern struct nemochannel *nemochannel_create(struct nemotool *tool);
extern void nemochannel_destroy(struct nemochannel *chan);

extern void nemochannel_dispatch(struct nemochannel *chan, uint64_t event);

static inline void nemochannel_set_dispatch_event(struct nemochannel *chan, nemochannel_dispatch_event_t dispatch)
{
	chan->dispatch_event = dispatch;
}

static inline void nemochannel_set_userdata(struct nemochannel *chan, void *data)
{
	chan->userdata = data;
}

static inline void *nemochannel_get_userdata(struct nemochannel *chan)
{
	return chan->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
