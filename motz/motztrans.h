#ifndef	__NEMOMOTZ_TRANSITION_H__
#define	__NEMOMOTZ_TRANSITION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>
#include <nemoease.h>

struct nemomotz;
struct motzone;
struct transone;

struct motztrans {
	struct nemolist link;

	struct nemoease ease;
	uint32_t duration;
	uint32_t delay;
	uint32_t stime;
	uint32_t etime;

	struct transone **ones;
	int nones, sones;
};

extern struct motztrans *nemomotz_transition_create(int max, int type, uint32_t duration, uint32_t delay);
extern void nemomotz_transition_destroy(struct motztrans *trans);

extern void nemomotz_transition_ease_set_type(struct motztrans *trans, int type);
extern void nemomotz_transition_ease_set_bezier(struct motztrans *trans, double x0, double y0, double x1, double y1);

extern int nemomotz_transition_set_attr(struct motztrans *trans, int index, float *toattr, float attr, uint32_t *todirty, uint32_t dirty);
extern void nemomotz_transition_put_attr(struct motztrans *trans, void *var, int size);
extern void nemomotz_transition_set_target(struct motztrans *trans, int index, float t, float v);

extern int nemomotz_transition_dispatch(struct motztrans *trans, uint32_t msecs);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
