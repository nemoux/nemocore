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

struct nemoaction {
	struct nemolist one_list;
	struct nemolist tap_list;
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

extern void nemoaction_set_one_tap_callback(struct nemoaction *action, void *target, nemoaction_tap_dispatch_event_t dispatch);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
