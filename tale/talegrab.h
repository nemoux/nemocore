#ifndef	__NEMOTALE_GRAB_H__
#define	__NEMOTALE_GRAB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>
#include <nemolistener.h>

#include <nemotale.h>
#include <taleevent.h>

struct talegrab;

typedef int (*nemotale_grab_dispatch_event_t)(struct talegrab *grab, struct taleevent *event);

struct talegrab {
	struct nemotale *tale;

	uint64_t device;

	struct nemolist link;

	struct nemolistener tale_destroy_listener;

	nemotale_grab_dispatch_event_t dispatch_event;
};

extern int nemotale_grab_prepare(struct talegrab *grab, struct nemotale *tale, struct taleevent *event, nemotale_grab_dispatch_event_t dispatch);
extern void nemotale_grab_finish(struct talegrab *grab);

extern struct talegrab *nemotale_grab_create(struct nemotale *tale, struct taleevent *event, nemotale_grab_dispatch_event_t dispatch);
extern void nemotale_grab_destroy(struct talegrab *grab);

static inline int nemotale_dispatch_grab(struct nemotale *tale, struct taleevent *event)
{
	struct talegrab *grab;

	nemolist_for_each(grab, &tale->grab_list, link) {
		if (grab->device == event->device)
			return grab->dispatch_event(grab, event);
	}

	return 0;
}

static inline void nemotale_dispatch_grab_all(struct nemotale *tale, struct taleevent *event)
{
	struct talegrab *grab, *next;
	int i;

	for (i = 0; i < event->tapcount; i++) {
		nemolist_for_each_safe(grab, next, &tale->grab_list, link) {
			if (grab->device == event->taps[i]->device)
				grab->dispatch_event(grab, event);
		}
	}
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
