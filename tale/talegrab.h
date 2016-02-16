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

typedef int (*nemotale_grab_dispatch_event_t)(void *data, uint32_t tag, void *event);

struct talegrab {
	struct nemotale *tale;

	uint64_t device;
	uint32_t tag;
	void *data;

	struct nemolist link;

	struct nemolistener destroy_listener;
	struct nemolistener tale_destroy_listener;

	nemotale_grab_dispatch_event_t dispatch_event;
};

extern int nemotale_grab_prepare(struct talegrab *grab, struct nemotale *tale, struct taleevent *event, nemotale_grab_dispatch_event_t dispatch);
extern void nemotale_grab_finish(struct talegrab *grab);

extern struct talegrab *nemotale_grab_create(struct nemotale *tale, struct taleevent *event, nemotale_grab_dispatch_event_t dispatch);
extern void nemotale_grab_destroy(struct talegrab *grab);

extern void nemotale_grab_check_signal(struct talegrab *grab, struct nemosignal *signal);

static inline void nemotale_grab_set_userdata(struct talegrab *grab, void *data)
{
	grab->data = data;
}

static inline void nemotale_grab_set_tag(struct talegrab *grab, uint32_t tag)
{
	grab->tag = tag;
}

static inline int nemotale_dispatch_grab(struct nemotale *tale, struct taleevent *event)
{
	struct talegrab *grab;

	nemolist_for_each(grab, &tale->grab_list, link) {
		if (grab->device == event->device) {
			int r;

			r = grab->dispatch_event(grab->data, grab->tag, event);
			if (r == 0) {
				nemotale_grab_destroy(grab);

				return 0;
			}

			return r;
		}
	}

	return 0;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
