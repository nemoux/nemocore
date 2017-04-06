#ifndef __NEMO_TRANSITION_H__
#define __NEMO_TRANSITION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoease.h>
#include <nemolist.h>
#include <nemolistener.h>

struct nemotransition;
struct transitionone;
struct transitionsensor;
struct transitiongroup;

typedef void (*nemotransition_dispatch_update_t)(struct nemotransition *trans, void *data, float t);
typedef void (*nemotransition_dispatch_done_t)(struct nemotransition *trans, void *data);

struct nemotransition {
	struct nemolist link;

	struct nemoease ease;

	struct transitionone **ones;
	int nones, sones;

	struct nemolist sensor_list;

	uint32_t duration;
	uint32_t delay;

	uint32_t stime;
	uint32_t etime;

	uint32_t repeat;
	uint32_t done;

	uint32_t tag;
	uint32_t flags;

	nemotransition_dispatch_update_t dispatch_update;
	nemotransition_dispatch_done_t dispatch_done;

	void *data;
};

struct transitiongroup {
	struct nemolist list;
};

extern struct transitiongroup *nemotransition_group_create(void);
extern void nemotransition_group_destroy(struct transitiongroup *group);

extern void nemotransition_group_attach_transition(struct transitiongroup *group, struct nemotransition *trans);
extern void nemotransition_group_detach_transition(struct transitiongroup *group, struct nemotransition *trans);

extern void nemotransition_group_ready(struct transitiongroup *group, uint32_t msecs);
extern int nemotransition_group_dispatch(struct transitiongroup *group, uint32_t msecs);

extern struct nemotransition *nemotransition_group_get_last_one(struct transitiongroup *group, void *var);
extern struct nemotransition *nemotransition_group_get_last_tag(struct transitiongroup *group, uint32_t tag);
extern struct nemotransition *nemotransition_group_get_last_all(struct transitiongroup *group);

extern void nemotransition_group_revoke_one(struct transitiongroup *group, void *var, int size);
extern void nemotransition_group_revoke_tag(struct transitiongroup *group, uint32_t tag);
extern void nemotransition_group_revoke_all(struct transitiongroup *group);

extern struct nemotransition *nemotransition_create(int max, int type, uint32_t duration, uint32_t delay);
extern void nemotransition_destroy(struct nemotransition *trans);

extern void nemotransition_set_tag(struct nemotransition *trans, uint32_t tag);

extern void nemotransition_ease_set_type(struct nemotransition *trans, int type);
extern void nemotransition_ease_set_bezier(struct nemotransition *trans, double x0, double y0, double x1, double y1);

extern void nemotransition_set_repeat(struct nemotransition *trans, uint32_t repeat);
extern int nemotransition_put_repeat(struct nemotransition *trans);

extern int nemotransition_ready(struct nemotransition *trans, uint32_t msecs);
extern int nemotransition_dispatch(struct nemotransition *trans, uint32_t msecs);

extern void nemotransition_set_float(struct nemotransition *trans, int index, float *var, float attr);
extern void nemotransition_set_double(struct nemotransition *trans, int index, double *var, double attr);
extern void nemotransition_set_float_with_dirty(struct nemotransition *trans, int index, float *var, float attr, uint32_t *obj, uint32_t dirty);
extern void nemotransition_set_double_with_dirty(struct nemotransition *trans, int index, double *var, double attr, uint32_t *obj, uint32_t dirty);
extern float nemotransition_get_float(struct nemotransition *trans, int index);
extern double nemotransition_get_double(struct nemotransition *trans, int index);

extern void nemotransition_set_target(struct nemotransition *trans, int index, double t, double v);

extern void nemotransition_put_one(struct nemotransition *trans, void *var, int size);

extern void nemotransition_check_object_destroy(struct nemotransition *trans, struct nemosignal *signal);
extern void nemotransition_check_object_revoke(struct nemotransition *trans, struct nemosignal *signal, void *var, int size);

static inline int nemotransition_group_has_transition(struct transitiongroup *group)
{
	return nemolist_empty(&group->list) == 0;
}

static inline uint32_t nemotransition_get_duration(struct nemotransition *trans)
{
	return trans->duration;
}

static inline uint32_t nemotransition_get_delay(struct nemotransition *trans)
{
	return trans->delay;
}

static inline void nemotransition_terminate(struct nemotransition *trans)
{
	trans->done = 1;
}

static inline void nemotransition_set_flags(struct nemotransition *trans, uint32_t flags)
{
	trans->flags |= flags;
}

static inline void nemotransition_put_flags(struct nemotransition *trans, uint32_t flags)
{
	trans->flags &= ~flags;
}

static inline int nemotransition_has_flags(struct nemotransition *trans, uint32_t flags)
{
	return trans->flags & flags;
}

static inline int nemotransition_has_flags_all(struct nemotransition *trans, uint32_t flags)
{
	return (trans->flags & flags) == flags;
}

static inline void nemotransition_set_dispatch_update(struct nemotransition *trans, nemotransition_dispatch_update_t dispatch)
{
	trans->dispatch_update = dispatch;
}

static inline void nemotransition_set_dispatch_done(struct nemotransition *trans, nemotransition_dispatch_done_t dispatch)
{
	trans->dispatch_done = dispatch;
}

static inline void nemotransition_set_userdata(struct nemotransition *trans, void *data)
{
	trans->data = data;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
