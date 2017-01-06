#ifndef __NEMOCOOK_TRANSFORM_H__
#define __NEMOCOOK_TRANSFORM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemomatrix.h>

struct cooktrans {
	float tx, ty, tz;
	float sx, sy, sz;
	float rx, ry, rz;

	struct nemomatrix matrix;
	struct nemomatrix inverse;

	struct cooktrans *parent;
};

extern struct cooktrans *nemocook_transform_create(void);
extern void nemocook_transform_destroy(struct cooktrans *trans);

extern int nemocook_transform_update(struct cooktrans *trans);

extern int nemocook_transform_to_global(struct cooktrans *trans, float sx, float sy, float sz, float *x, float *y, float *z);
extern int nemocook_transform_from_global(struct cooktrans *trans, float x, float y, float z, float *sx, float *sy, float *sz);
extern int nemocook_2d_transform_to_global(struct cooktrans *trans, float sx, float sy, float *x, float *y);
extern int nemocook_2d_transform_from_global(struct cooktrans *trans, float x, float y, float *sx, float *sy);

static inline void nemocook_transform_set_translate(struct cooktrans *trans, float tx, float ty, float tz)
{
	trans->tx = tx;
	trans->ty = ty;
	trans->tz = tz;
}

static inline void nemocook_transform_set_scale(struct cooktrans *trans, float sx, float sy, float sz)
{
	trans->sx = sx;
	trans->sy = sy;
	trans->sz = sz;
}

static inline void nemocook_transform_set_rotate(struct cooktrans *trans, float rx, float ry, float rz)
{
	trans->rx = rx;
	trans->ry = ry;
	trans->rz = rz;
}

static inline void nemocook_transform_set_parent(struct cooktrans *trans, struct cooktrans *parent)
{
	trans->parent = parent;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
