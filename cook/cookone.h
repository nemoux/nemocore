#ifndef __NEMOCOOK_ONE_H__
#define __NEMOCOOK_ONE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

struct cookstate;

struct cookone {
	struct nemolist list;
};

extern int nemocook_one_prepare(struct cookone *one);
extern void nemocook_one_finish(struct cookone *one);

extern void nemocook_one_attach_state(struct cookone *one, struct cookstate *state);
extern void nemocook_one_detach_state(struct cookone *one, int tag);

extern void nemocook_one_update_state(struct cookone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
