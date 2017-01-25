#ifndef __NEMO_TRANS_H__
#define __NEMO_TRANS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoease.h>
#include <nemoattr.h>
#include <nemolist.h>

#define NEMOTRANS_ONE_TARGETS_MAX			(8)

struct nemotrans;
struct transgroup;

typedef void (*nemotrans_dispatch_update_t)(struct nemotrans *trans, void *data, float t);
typedef void (*nemotrans_dispatch_done_t)(struct nemotrans *trans, void *data);

struct transone {
	struct nemoattr attr;
	int is_double;

	double timings[NEMOTRANS_ONE_TARGETS_MAX];
	double targets[NEMOTRANS_ONE_TARGETS_MAX];
	int count;
};

struct nemotrans {
	struct nemolist link;

	struct nemoease ease;

	struct transone **ones;
	int nones;

	uint32_t duration;
	uint32_t delay;

	uint32_t stime;
	uint32_t etime;

	uint32_t tag;
	uint32_t flags;

	nemotrans_dispatch_update_t dispatch_update;
	nemotrans_dispatch_done_t dispatch_done;

	void *data;
};

struct transgroup {
	struct nemolist list;
};

extern struct transgroup *nemotrans_group_create(void);
extern void nemotrans_group_destroy(struct transgroup *group);

extern void nemotrans_group_attach_trans(struct transgroup *group, struct nemotrans *trans);
extern void nemotrans_group_detach_trans(struct transgroup *group, struct nemotrans *trans);

extern void nemotrans_group_ready(struct transgroup *group, uint32_t msecs);
extern void nemotrans_group_dispatch(struct transgroup *group, uint32_t msecs);

extern struct nemotrans *nemotrans_group_get_last_one(struct transgroup *group, void *var);
extern struct nemotrans *nemotrans_group_get_last_tag(struct transgroup *group, uint32_t tag);
extern struct nemotrans *nemotrans_group_get_last_all(struct transgroup *group);

extern void nemotrans_group_revoke_one(struct transgroup *group, void *var);
extern void nemotrans_group_revoke_tag(struct transgroup *group, uint32_t tag);
extern void nemotrans_group_revoke_all(struct transgroup *group);

extern struct nemotrans *nemotrans_create(int max, int type, uint32_t duration, uint32_t delay);
extern void nemotrans_destroy(struct nemotrans *trans);

extern void nemotrans_set_tag(struct nemotrans *trans, uint32_t tag);

extern void nemotrans_ease_set_type(struct nemotrans *trans, int type);
extern void nemotrans_ease_set_bezier(struct nemotrans *trans, double x0, double y0, double x1, double y1);

extern int nemotrans_ready(struct nemotrans *trans, uint32_t msecs);
extern int nemotrans_dispatch(struct nemotrans *trans, uint32_t msecs);

extern void nemotrans_set_float(struct nemotrans *trans, int index, float *var);
extern void nemotrans_set_double(struct nemotrans *trans, int index, double *var);
extern float nemotrans_get_float(struct nemotrans *trans, int index);
extern double nemotrans_get_double(struct nemotrans *trans, int index);

extern void nemotrans_set_target(struct nemotrans *trans, int index, double t, double v);

extern void nemotrans_set_dispatch_update(struct nemotrans *trans, nemotrans_dispatch_update_t dispatch);
extern void nemotrans_set_dispatch_done(struct nemotrans *trans, nemotrans_dispatch_done_t dispatch);
extern void nemotrans_set_userdata(struct nemotrans *trans, void *data);

static inline int nemotrans_group_has_transition(struct transgroup *group)
{
	return nemolist_empty(&group->list) == 0;
}

static inline uint32_t nemotrans_get_duration(struct nemotrans *trans)
{
	return trans->duration;
}

static inline uint32_t nemotrans_get_delay(struct nemotrans *trans)
{
	return trans->delay;
}

static inline void nemotrans_set_flags(struct nemotrans *trans, uint32_t flags)
{
	trans->flags |= flags;
}

static inline void nemotrans_put_flags(struct nemotrans *trans, uint32_t flags)
{
	trans->flags &= ~flags;
}

static inline int nemotrans_has_flags(struct nemotrans *trans, uint32_t flags)
{
	return trans->flags & flags;
}

static inline int nemotrans_has_flags_all(struct nemotrans *trans, uint32_t flags)
{
	return (trans->flags & flags) == flags;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
