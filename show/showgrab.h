#ifndef __NEMOSHOW_GRAB_H__
#define __NEMOSHOW_GRAB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>
#include <nemolistener.h>

#include <taleevent.h>
#include <talegesture.h>
#include <talegrab.h>

struct showgrab;

typedef int (*nemoshow_grab_dispatch_event_t)(struct nemoshow *show, struct showgrab *grab, void *event);

struct showgrab {
	struct talegrab base;

	struct nemoshow *show;

	nemoshow_grab_dispatch_event_t dispatch_event;
	uint32_t tag;
	void *data;

	struct nemolistener destroy_listener;
};

extern struct showgrab *nemoshow_grab_create(struct nemoshow *show, void *event, nemoshow_grab_dispatch_event_t dispatch);
extern void nemoshow_grab_destroy(struct showgrab *grab);

extern void nemoshow_grab_check_signal(struct showgrab *grab, struct nemosignal *signal);

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

static inline void nemoshow_dispatch_grab(struct nemoshow *show, void *event)
{
	nemotale_dispatch_grab(show->tale, (struct taleevent *)event);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
