#ifndef __NEMO_CHANNEL_H__
#define	__NEMO_CHANNEL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemochannel {
	int eventfd;

	void *userdata;
};

extern struct nemochannel *nemochannel_create(void);
extern void nemochannel_destroy(struct nemochannel *chan);

extern void nemochannel_set_blocking_mode(struct nemochannel *chan, int is_blocking);

extern uint64_t nemochannel_read(struct nemochannel *chan);
extern void nemochannel_write(struct nemochannel *chan, uint64_t event);

static inline int nemochannel_get_fd(struct nemochannel *chan)
{
	return chan->eventfd;
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
