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

struct nemotrans;
struct transgroup;

typedef void (*nemotrans_dispatch_update_t)(struct nemotrans *trans, void *data);
typedef void (*nemotrans_dispatch_done_t)(struct nemotrans *trans, void *data);
typedef void (*nemotrans_group_dispatch_first_t)(struct transgroup *group, void *data);
typedef void (*nemotrans_group_dispatch_last_t)(struct transgroup *group, void *data);

struct transone {
	struct nemolist link;

	struct nemoattr attr;
	int is_double;

	double sattr;
	double eattr;
};

struct nemotrans {
	struct nemolist link;

	struct nemoease ease;

	struct nemolist list;

	uint32_t duration;
	uint32_t delay;

	uint32_t stime;
	uint32_t etime;

	uint32_t tag;

	nemotrans_dispatch_update_t dispatch_update;
	nemotrans_dispatch_done_t dispatch_done;

	void *data;
};

struct transgroup {
	struct nemolist list;

	nemotrans_group_dispatch_first_t dispatch_first;
	nemotrans_group_dispatch_last_t dispatch_last;

	void *data;
};

extern struct transgroup *nemotrans_group_create(void);
extern void nemotrans_group_destroy(struct transgroup *group);

extern void nemotrans_group_attach_trans(struct transgroup *group, struct nemotrans *trans);
extern void nemotrans_group_detach_trans(struct transgroup *group, struct nemotrans *trans);

extern void nemotrans_group_set_dispatch_first(struct transgroup *group, nemotrans_group_dispatch_first_t dispatch);
extern void nemotrans_group_set_dispatch_last(struct transgroup *group, nemotrans_group_dispatch_last_t dispatch);
extern void nemotrans_group_set_userdata(struct transgroup *group, void *data);

extern void nemotrans_group_ready(struct transgroup *group, uint32_t msecs);
extern void nemotrans_group_dispatch(struct transgroup *group, uint32_t msecs);

extern struct nemotrans *nemotrans_group_get_last_one(struct transgroup *group, void *var);
extern struct nemotrans *nemotrans_group_get_last_tag(struct transgroup *group, uint32_t tag);
extern struct nemotrans *nemotrans_group_get_last_all(struct transgroup *group);

extern void nemotrans_group_remove_one(struct transgroup *group, void *var);
extern void nemotrans_group_remove_tag(struct transgroup *group, uint32_t tag);
extern void nemotrans_group_remove_all(struct transgroup *group);

extern struct nemotrans *nemotrans_create(int type, uint32_t duration, uint32_t delay);
extern void nemotrans_destroy(struct nemotrans *trans);

extern void nemotrans_set_tag(struct nemotrans *trans, uint32_t tag);

extern void nemotrans_ease_set_type(struct nemotrans *trans, int type);
extern void nemotrans_ease_set_bezier(struct nemotrans *trans, double x0, double y0, double x1, double y1);

extern int nemotrans_ready(struct nemotrans *trans, uint32_t msecs);
extern int nemotrans_dispatch(struct nemotrans *trans, uint32_t msecs);

extern void nemotrans_set_float(struct nemotrans *trans, float *var, float value);
extern void nemotrans_set_double(struct nemotrans *trans, double *var, double value);

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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
