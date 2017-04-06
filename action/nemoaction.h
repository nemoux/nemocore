#ifndef __NEMOACTION_H__
#define __NEMOACTION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <actionone.h>
#include <actiontap.h>

typedef enum {
	NEMOACTION_TAP_DOWN_EVENT = (1 << 0),
	NEMOACTION_TAP_UP_EVENT = (1 << 1),
	NEMOACTION_TAP_MOTION_EVENT = (1 << 2)
} NemoActionEventType;

struct nemoaction {
	struct nemolist one_list;
	struct nemolist tap_list;

	nemoaction_tap_dispatch_event_t dispatch_tap_event;

	void *data;
};

extern struct nemoaction *nemoaction_create(void);
extern void nemoaction_destroy(struct nemoaction *action);

extern void nemoaction_destroy_one_all(struct nemoaction *action);
extern void nemoaction_destroy_one_by_target(struct nemoaction *action, void *target);
extern void nemoaction_destroy_tap_all(struct nemoaction *action);
extern void nemoaction_destroy_tap_by_target(struct nemoaction *action, void *target);

extern struct actionone *nemoaction_get_one_by_target(struct nemoaction *action, void *target);

extern struct actiontap *nemoaction_get_tap_by_device(struct nemoaction *action, uint64_t device);
extern struct actiontap *nemoaction_get_tap_by_serial(struct nemoaction *action, uint32_t serial);

extern int nemoaction_get_taps_by_target(struct nemoaction *action, void *target, struct actiontap **taps, int mtaps);
extern int nemoaction_get_taps_all(struct nemoaction *action, struct actiontap **taps, int mtaps);

extern int nemoaction_get_distant_taps(struct nemoaction *action, struct actiontap **taps, int ntaps, int *index0, int *index1);

static inline void nemoaction_set_tap_callback(struct nemoaction *action, nemoaction_tap_dispatch_event_t dispatch)
{
	action->dispatch_tap_event = dispatch;
}

static inline void nemoaction_set_userdata(struct nemoaction *action, void *data)
{
	action->data = data;
}

static inline void *nemoaction_get_userdata(struct nemoaction *action)
{
	return action->data;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
