#ifndef	__NEMOYOYO_TRANSITION_H__
#define	__NEMOYOYO_TRANSITION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>
#include <nemoease.h>

struct nemoyoyo;
struct yoyoone;
struct yoyotransition;
struct transitionone;

typedef void (*nemoyoyo_transition_dispatch_update_t)(struct yoyotransition *trans, void *data, float t);
typedef void (*nemoyoyo_transition_dispatch_done_t)(struct yoyotransition *trans, void *data);

struct yoyotransition {
	struct nemolist link;

	struct nemoease ease;
	uint32_t duration;
	uint32_t delay;
	uint32_t stime;
	uint32_t etime;

	uint32_t repeat;

	struct transitionone **ones;
	int nones, sones;

	struct nemolist sensor_list;

	nemoyoyo_transition_dispatch_update_t dispatch_update;
	nemoyoyo_transition_dispatch_done_t dispatch_done;

	void *data;
};

#define NEMOYOYO_ONE_DECLARE_SET_TRANSITION(type, attr, name, _dirty)	\
	static inline void nemoyoyo_transition_one_set_##name(struct yoyotransition *trans, int index, struct yoyoone *one) {	\
		nemoyoyo_transition_set_attr(trans, index, &one->attr, one->attr, &one->dirty, _dirty);	\
	}
#define NEMOYOYO_ONE_DECLARE_SET_TRANSITION_WITH_FLAGS(type, attr, name, _dirty, _flags)	\
	static inline void nemoyoyo_transition_one_set_##name(struct yoyotransition *trans, int index, struct yoyoone *one) {	\
		nemoyoyo_one_set_flags(one, _flags);	\
		nemoyoyo_transition_set_attr(trans, index, &one->attr, one->attr, &one->dirty, _dirty);	\
	}

#define NEMOYOYO_TAG_DECLARE_SET_TRANSITION(tag, type, attr, name, _dirty)	\
	static inline void nemoyoyo_transition_##tag##_set_##name(struct yoyotransition *trans, int index, struct yoyoone *one) {	\
		struct yoyo##tag *tag = ((struct yoyo##tag *)container_of(one, struct yoyo##tag, one));	\
		nemoyoyo_transition_set_attr(trans, index, &tag->attr, tag->attr, &one->dirty, _dirty);	\
	}
#define NEMOYOYO_TAG_DECLARE_SET_TRANSITION_WITH_FLAGS(tag, type, attr, name, _dirty, _flags)	\
	static inline void nemoyoyo_transition_##tag##_set_##name(struct yoyotransition *trans, int index, struct yoyoone *one) {	\
		struct yoyo##tag *tag = ((struct yoyo##tag *)container_of(one, struct yoyo##tag, one));	\
		nemoyoyo_one_set_flags(one, _flags);	\
		nemoyoyo_transition_set_attr(trans, index, &tag->attr, tag->attr, &one->dirty, _dirty);	\
	}

extern struct yoyotransition *nemoyoyo_transition_create(int max, int type, uint32_t duration, uint32_t delay);
extern void nemoyoyo_transition_destroy(struct yoyotransition *trans);

extern void nemoyoyo_transition_ease_set_type(struct yoyotransition *trans, int type);
extern void nemoyoyo_transition_ease_set_bezier(struct yoyotransition *trans, double x0, double y0, double x1, double y1);

extern void nemoyoyo_transition_set_repeat(struct yoyotransition *trans, uint32_t repeat);
extern int nemoyoyo_transition_check_repeat(struct yoyotransition *trans);

extern void nemoyoyo_transition_check_one(struct yoyotransition *trans, struct yoyoone *one);

extern int nemoyoyo_transition_set_attr(struct yoyotransition *trans, int index, float *toattr, float attr, uint32_t *todirty, uint32_t dirty);
extern void nemoyoyo_transition_put_attr(struct yoyotransition *trans, void *var, int size);
extern void nemoyoyo_transition_set_target(struct yoyotransition *trans, int index, float t, float v);

extern int nemoyoyo_transition_dispatch(struct yoyotransition *trans, uint32_t msecs);

static inline void nemoyoyo_transition_set_dispatch_update(struct yoyotransition *trans, nemoyoyo_transition_dispatch_update_t dispatch)
{
	trans->dispatch_update = dispatch;
}

static inline void nemoyoyo_transition_set_dispatch_done(struct yoyotransition *trans, nemoyoyo_transition_dispatch_done_t dispatch)
{
	trans->dispatch_done = dispatch;
}

static inline void nemoyoyo_transition_set_userdata(struct yoyotransition *trans, void *data)
{
	trans->data = data;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
