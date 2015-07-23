#ifndef	__NEMOTALE_GRAB_H__
#define	__NEMOTALE_GRAB_H__

#include <stdint.h>

#include <nemolist.h>
#include <nemolistener.h>

#include <nemotale.h>
#include <talenode.h>

struct talegrab;

typedef int (*nemotale_dispatch_grab_t)(struct talegrab *grab, uint32_t type, struct taleevent *event);

struct talegrab {
	struct nemotale *tale;
	struct talenode *node;

	uint64_t device;

	struct nemolist link;

	struct nemolistener tale_destroy_listener;
	struct nemolistener node_destroy_listener;

	nemotale_dispatch_grab_t dispatch;
};

extern int nemotale_prepare_grab(struct talegrab *grab, struct nemotale *tale, struct talenode *node, uint64_t device, nemotale_dispatch_grab_t dispatch);
extern void nemotale_finish_grab(struct talegrab *grab);

extern struct talegrab *nemotale_create_grab(struct nemotale *tale, struct talenode *node, uint64_t device, nemotale_dispatch_grab_t dispatch);
extern void nemotale_destroy_grab(struct talegrab *grab);
extern void nemotale_destroy_grab_with_device(struct nemotale *tale, uint64_t device);

static inline int nemotale_dispatch_grab(struct nemotale *tale, uint64_t device, uint32_t type, struct taleevent *event)
{
	struct talegrab *grab;

	nemolist_for_each(grab, &tale->grab_list, link) {
		if (grab->device == device) {
			return grab->dispatch(grab, type, event);
		}
	}

	return 0;
}

#endif
