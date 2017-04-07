#ifndef __NEMOYOYO_ONE_H__
#define __NEMOYOYO_ONE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemocook.h>
#include <nemolist.h>
#include <nemolistener.h>
#include <nemotransition.h>

#include <nemoyoyo.h>

typedef enum {
	NEMOYOYO_ONE_TRANSFORM_FLAG = (1 << 0),
	NEMOYOYO_ONE_PICK_FLAG = (1 << 1)
} NemoYoyoOneFlag;

typedef enum {
	NEMOYOYO_ONE_FLAGS_DIRTY = (1 << 0),
	NEMOYOYO_ONE_TRANSFORM_DIRTY = (1 << 1)
} NemoYoyoOneDirty;

struct yoyoone {
	struct cookpoly *poly;
	struct cooktrans *trans;
	struct cooktex *tex;

	uint32_t flags;
	uint32_t dirty;

	float alpha;

	struct {
		float tx, ty;
		float sx, sy;
		float rz;

		float ax, ay;

		float w, h;
	} geometry;

	pixman_region32_t bounds;

	struct nemolist link;
	struct nemosignal destroy_signal;

	void *data;
};

extern struct yoyoone *nemoyoyo_one_create(void);
extern void nemoyoyo_one_destroy(struct yoyoone *one);

extern void nemoyoyo_one_set_texture(struct yoyoone *one, struct cooktex *tex);

extern void nemoyoyo_one_update(struct nemoyoyo *yoyo, struct yoyoone *one);

extern int nemoyoyo_one_clip_slice(struct yoyoone *one, pixman_region32_t *region, float *vertices, float *texcoords, int *slices);

static inline void nemoyoyo_one_set_dirty(struct yoyoone *one, uint32_t dirty)
{
	one->dirty |= dirty;
}

static inline void nemoyoyo_one_put_dirty(struct yoyoone *one, uint32_t dirty)
{
	one->dirty &= ~dirty;
}

static inline void nemoyoyo_one_put_dirty_all(struct yoyoone *one)
{
	one->dirty = 0x0;
}

static inline int nemoyoyo_one_has_dirty(struct yoyoone *one, uint32_t dirty)
{
	return one->dirty & dirty;
}

static inline int nemoyoyo_one_has_dirty_all(struct yoyoone *one, uint32_t dirty)
{
	return (one->dirty & dirty) == dirty;
}

static inline int nemoyoyo_one_has_no_dirty(struct yoyoone *one)
{
	return one->dirty == 0x0;
}

static inline void nemoyoyo_one_set_flags(struct yoyoone *one, uint32_t flags)
{
	one->flags |= flags;

	nemoyoyo_one_set_dirty(one, NEMOYOYO_ONE_FLAGS_DIRTY);
}

static inline void nemoyoyo_one_put_flags(struct yoyoone *one, uint32_t flags)
{
	one->flags &= ~flags;

	nemoyoyo_one_set_dirty(one, NEMOYOYO_ONE_FLAGS_DIRTY);
}

static inline int nemoyoyo_one_has_flags(struct yoyoone *one, uint32_t flags)
{
	return one->flags & flags;
}

static inline int nemoyoyo_one_has_flags_all(struct yoyoone *one, uint32_t flags)
{
	return (one->flags & flags) == flags;
}

static inline void nemoyoyo_one_set_userdata(struct yoyoone *one, void *data)
{
	one->data = data;
}

static inline void *nemoyoyo_one_get_userdata(struct yoyoone *one)
{
	return one->data;
}

#define NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE(type, attr, name, dirty)	\
	static inline void nemoyoyo_one_set_##name(struct yoyoone *one, type name) {	\
		one->attr = name;	\
		nemoyoyo_one_set_dirty(one, dirty);	\
	}
#define NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(type, attr, name, dirty, flags)	\
	static inline void nemoyoyo_one_set_##name(struct yoyoone *one, type name) {	\
		one->attr = name;	\
		nemoyoyo_one_set_dirty(one, dirty);	\
		nemoyoyo_one_set_flags(one, flags);	\
	}
#define NEMOYOYO_ONE_DECLARE_GET_ATTRIBUTE(type, attr, name)	\
	static inline type nemoyoyo_one_get_##name(struct yoyoone *one) {	\
		return one->attr;	\
	}
#define NEMOYOYO_ONE_DECLARE_DUP_STRING(attr, name, dirty)	\
	static inline void nemoyoyo_one_set_##name(struct yoyoone *one, const char *attr) {	\
		if (one->attr != NULL) free(one->attr);	\
		one->attr = strdup(attr);	\
		nemoyoyo_one_set_dirty(one, dirty);	\
	}
#define NEMOYOYO_ONE_DECLARE_COPY_STRING(attr, name, dirty)	\
	static inline void nemoyoyo_one_set_##name(struct yoyoone *one, const char *attr) {	\
		strcpy(one->attr, attr);	\
		nemoyoyo_one_set_dirty(one, dirty);	\
	}
#define NEMOYOYO_ONE_DECLARE_SET_TRANSITION(attr, name, _dirty)	\
	static inline void nemoyoyo_one_transition_set_##name(struct nemotransition *trans, int index, struct yoyoone *one) {	\
		nemotransition_set_float_with_dirty(trans, index, &one->attr, one->attr, &one->dirty, _dirty);	\
	}
#define NEMOYOYO_ONE_DECLARE_SET_TRANSITION_WITH_FLAGS(attr, name, _dirty, _flags)	\
	static inline void nemoyoyo_one_transition_set_##name(struct nemotransition *trans, int index, struct yoyoone *one) {	\
		nemoyoyo_one_set_flags(one, _flags);	\
		nemotransition_set_float_with_dirty(trans, index, &one->attr, one->attr, &one->dirty, _dirty);	\
	}
#define NEMOYOYO_ONE_DECLARE_CHECK_TRANSITION_DESTROY()	\
	static inline void nemoyoyo_one_transition_check_destroy(struct nemotransition *trans, struct yoyoone *one) {	\
		nemotransition_check_object_destroy(trans, &one->destroy_signal);	\
	}
#define NEMOYOYO_ONE_DECLARE_CHECK_TRANSITION_REVOKE()	\
	static inline void nemoyoyo_one_transition_check_revoke(struct nemotransition *trans, struct yoyoone *one) {	\
		nemotransition_check_object_revoke(trans, &one->destroy_signal, one, sizeof(struct yoyoone));	\
	}

NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE(float, alpha, alpha, 0x0);

NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE(float, geometry.tx, tx, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE(float, geometry.ty, ty, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, geometry.sx, sx, NEMOYOYO_ONE_TRANSFORM_DIRTY, NEMOYOYO_ONE_TRANSFORM_FLAG);
NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, geometry.sy, sy, NEMOYOYO_ONE_TRANSFORM_DIRTY, NEMOYOYO_ONE_TRANSFORM_FLAG);
NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, geometry.rz, rz, NEMOYOYO_ONE_TRANSFORM_DIRTY, NEMOYOYO_ONE_TRANSFORM_FLAG);
NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE(float, geometry.w, width, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE(float, geometry.h, height, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE(float, geometry.ax, ax, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_ATTRIBUTE(float, geometry.ay, ay, NEMOYOYO_ONE_TRANSFORM_DIRTY);

NEMOYOYO_ONE_DECLARE_SET_TRANSITION(alpha, alpha, 0x0);

NEMOYOYO_ONE_DECLARE_SET_TRANSITION(geometry.tx, tx, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_TRANSITION(geometry.ty, ty, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_TRANSITION_WITH_FLAGS(geometry.sx, sx, NEMOYOYO_ONE_TRANSFORM_DIRTY, NEMOYOYO_ONE_TRANSFORM_FLAG);
NEMOYOYO_ONE_DECLARE_SET_TRANSITION_WITH_FLAGS(geometry.sy, sy, NEMOYOYO_ONE_TRANSFORM_DIRTY, NEMOYOYO_ONE_TRANSFORM_FLAG);
NEMOYOYO_ONE_DECLARE_SET_TRANSITION_WITH_FLAGS(geometry.rz, rz, NEMOYOYO_ONE_TRANSFORM_DIRTY, NEMOYOYO_ONE_TRANSFORM_FLAG);
NEMOYOYO_ONE_DECLARE_SET_TRANSITION(geometry.w, width, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_TRANSITION(geometry.h, height, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_TRANSITION(geometry.ax, ax, NEMOYOYO_ONE_TRANSFORM_DIRTY);
NEMOYOYO_ONE_DECLARE_SET_TRANSITION(geometry.ay, ay, NEMOYOYO_ONE_TRANSFORM_DIRTY);

NEMOYOYO_ONE_DECLARE_CHECK_TRANSITION_DESTROY();
NEMOYOYO_ONE_DECLARE_CHECK_TRANSITION_REVOKE();

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
