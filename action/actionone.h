#ifndef __NEMOACTION_ONE_H__
#define __NEMOACTION_ONE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <actiontap.h>

struct nemoaction;

struct actionone {
	struct nemoaction *action;

	void *target;

	nemoaction_tap_dispatch_event_t dispatch_tap_event;

	struct nemolist link;
};

extern struct actionone *nemoaction_one_create(struct nemoaction *action);
extern void nemoaction_one_destroy(struct actionone *one);

static inline void nemoaction_one_set_tap_callback(struct actionone *one, nemoaction_tap_dispatch_event_t dispatch)
{
	one->dispatch_tap_event = dispatch;
}

static inline void nemoaction_one_set_target(struct actionone *one, void *target)
{
	one->target = target;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
