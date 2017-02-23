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
struct motztrans;
struct transone;

typedef void (*nemomotz_transition_dispatch_update_t)(struct motztrans *trans, void *data, float t);
typedef void (*nemomotz_transition_dispatch_done_t)(struct motztrans *trans, void *data);

struct motztrans {
	struct nemolist link;

	struct nemoease ease;
	uint32_t duration;
	uint32_t delay;
	uint32_t stime;
	uint32_t etime;

	uint32_t repeat;

	struct transone **ones;
	int nones, sones;

	nemomotz_transition_dispatch_update_t dispatch_update;
	nemomotz_transition_dispatch_done_t dispatch_done;

	void *data;
};

#define NEMOMOTZ_DECLARE_SET_TRANSITION(tag, type, attr, name, _dirty)	\
	static inline void nemomotz_transition_##tag##_set_##name(struct motztrans *trans, int index, struct motzone *one) {	\
		struct motz##tag *tag = ((struct motz##tag *)container_of(one, struct motz##tag, one));	\
		nemomotz_transition_set_attr(trans, index, &tag->attr, tag->attr, &one->dirty, _dirty);	\
	}
#define NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(tag, type, attr, name, _dirty, _flags)	\
	static inline void nemomotz_transition_##tag##_set_##name(struct motztrans *trans, int index, struct motzone *one) {	\
		struct motz##tag *tag = ((struct motz##tag *)container_of(one, struct motz##tag, one));	\
		nemomotz_one_set_flags(one, _flags);	\
		nemomotz_transition_set_attr(trans, index, &tag->attr, tag->attr, &one->dirty, _dirty);	\
	}

extern struct motztrans *nemomotz_transition_create(int max, int type, uint32_t duration, uint32_t delay);
extern void nemomotz_transition_destroy(struct motztrans *trans);

extern void nemomotz_transition_ease_set_type(struct motztrans *trans, int type);
extern void nemomotz_transition_ease_set_bezier(struct motztrans *trans, double x0, double y0, double x1, double y1);

extern void nemomotz_transition_set_repeat(struct motztrans *trans, uint32_t repeat);
extern int nemomotz_transition_check_repeat(struct motztrans *trans);

extern int nemomotz_transition_set_attr(struct motztrans *trans, int index, float *toattr, float attr, uint32_t *todirty, uint32_t dirty);
extern void nemomotz_transition_put_attr(struct motztrans *trans, void *var, int size);
extern void nemomotz_transition_set_target(struct motztrans *trans, int index, float t, float v);

extern int nemomotz_transition_dispatch(struct motztrans *trans, uint32_t msecs);

static inline void nemomotz_transition_set_dispatch_update(struct motztrans *trans, nemomotz_transition_dispatch_update_t dispatch)
{
	trans->dispatch_update = dispatch;
}

static inline void nemomotz_transition_set_dispatch_done(struct motztrans *trans, nemomotz_transition_dispatch_done_t dispatch)
{
	trans->dispatch_done = dispatch;
}

static inline void nemomotz_transition_set_userdata(struct motztrans *trans, void *data)
{
	trans->data = data;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
