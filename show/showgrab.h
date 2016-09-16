#ifndef __NEMOSHOW_GRAB_H__
#define __NEMOSHOW_GRAB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>
#include <nemolistener.h>

struct nemoshow;
struct showgrab;

typedef int (*nemoshow_grab_dispatch_event_t)(struct nemoshow *show, struct showgrab *grab, struct showevent *event);

struct showgrab {
	struct nemoshow *show;

	struct nemolist link;

	uint64_t device;

	nemoshow_grab_dispatch_event_t dispatch_event;
	uint32_t tag;
	void *data;

	struct nemolistener destroy_listener;
};

extern struct showgrab *nemoshow_grab_create(struct nemoshow *show, struct showevent *event, nemoshow_grab_dispatch_event_t dispatch);
extern void nemoshow_grab_destroy(struct showgrab *grab);

extern void nemoshow_grab_check_signal(struct showgrab *grab, struct nemosignal *signal);

extern int nemoshow_dispatch_grab(struct nemoshow *show, struct showevent *event);
extern void nemoshow_dispatch_grab_all(struct nemoshow *show, struct showevent *event);

static inline void nemoshow_grab_set_userdata(struct showgrab *grab, void *data)
{
	grab->data = data;
}

static inline void *nemoshow_grab_get_userdata(struct showgrab *grab)
{
	return grab->data;
}

static inline void nemoshow_grab_set_tag(struct showgrab *grab, uint32_t tag)
{
	grab->tag = tag;
}

static inline uint32_t nemoshow_grab_get_tag(struct showgrab *grab)
{
	return grab->tag;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
